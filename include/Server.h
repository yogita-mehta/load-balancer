#pragma once
#include <string>
#include <atomic>
#include <mutex>
#include <deque>
#include <random>
#include <algorithm>

enum class ServerHealth { HEALTHY, DEGRADED, DOWN };

// A simulated backend server. Thread-safe: multiple worker threads may
// dispatch/complete requests against the same server concurrently.
class Server {
public:
    Server(int id, int weight, int capacity)
        : id_(id), weight_(weight), capacity_(capacity),
          activeConnections_(0), health_(ServerHealth::HEALTHY),
          cpuUtil_(0.0), memUtil_(0.0), totalServed_(0), totalFailed_(0),
          rng_(std::random_device{}() + id) {}

    int id() const { return id_; }
    int weight() const { return weight_; }
    int capacity() const { return capacity_; }

    int activeConnections() const { return activeConnections_.load(std::memory_order_relaxed); }

    ServerHealth health() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return health_;
    }

    void setHealth(ServerHealth h) {
        std::lock_guard<std::mutex> lk(mutex_);
        health_ = h;
    }

    bool isAvailable() const {
        return health() != ServerHealth::DOWN && activeConnections() < capacity_;
    }

    // Called when a request is dispatched to this server.
    void beginRequest() {
        activeConnections_.fetch_add(1, std::memory_order_relaxed);
    }

    // Called when the server finishes (successfully or not) a request.
    // Updates rolling average response time (exponential moving average).
    void endRequest(double latencyMs, bool failed) {
        activeConnections_.fetch_sub(1, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lk(mutex_);
        if (failed) {
            totalFailed_++;
        } else {
            totalServed_++;
            constexpr double alpha = 0.2; // EMA smoothing factor
            avgResponseTimeMs_ = (avgResponseTimeMs_ == 0.0)
                ? latencyMs
                : (alpha * latencyMs + (1 - alpha) * avgResponseTimeMs_);
        }
        // Simulate CPU/memory load as a function of utilization ratio + noise.
        double util = static_cast<double>(activeConnections_.load()) / std::max(1, capacity_);
        std::uniform_real_distribution<double> noise(-5.0, 5.0);
        cpuUtil_ = std::clamp(util * 100.0 + noise(rng_), 0.0, 100.0);
        memUtil_ = std::clamp(util * 80.0 + noise(rng_), 0.0, 100.0);
    }

    double avgResponseTimeMs() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return avgResponseTimeMs_;
    }

    double cpuUtilization() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return cpuUtil_;
    }

    double memUtilization() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return memUtil_;
    }

    uint64_t totalServed() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return totalServed_;
    }

    uint64_t totalFailed() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return totalFailed_;
    }

private:
    int id_;
    int weight_;
    int capacity_;
    std::atomic<int> activeConnections_;

    mutable std::mutex mutex_;
    ServerHealth health_;
    double avgResponseTimeMs_ = 0.0;
    double cpuUtil_;
    double memUtil_;
    uint64_t totalServed_;
    uint64_t totalFailed_;
    std::mt19937 rng_;
};
