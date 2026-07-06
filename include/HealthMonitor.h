#pragma once
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <functional>
#include <unordered_map>
#include "Server.h"

// Background thread that simulates heartbeat checks: periodically it may
// randomly fail a healthy server (per failureProbability) and automatically
// recover down servers after recoveryTimeMs. Mirrors real-world liveness
// probing + auto-remediation.
class HealthMonitor {
public:
    HealthMonitor(std::vector<std::unique_ptr<Server>>& servers,
                  double failureProbability,
                  std::chrono::milliseconds checkInterval,
                  std::chrono::milliseconds recoveryTime,
                  std::function<void(int, const std::string&)> onEvent = nullptr)
        : servers_(servers),
          failureProbability_(failureProbability),
          checkInterval_(checkInterval),
          recoveryTime_(recoveryTime),
          onEvent_(std::move(onEvent)),
          rng_(std::random_device{}()) {}

    void start() {
        running_ = true;
        thread_ = std::thread([this] { run(); });
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    ~HealthMonitor() { stop(); }

private:
    void run() {
        std::uniform_real_distribution<double> chance(0.0, 1.0);
        while (running_) {
            std::this_thread::sleep_for(checkInterval_);
            if (!running_) break;
            auto now = std::chrono::steady_clock::now();
            for (auto& s : servers_) {
                if (s->health() == ServerHealth::DOWN) {
                    auto it = downSince_.find(s->id());
                    if (it != downSince_.end() &&
                        now - it->second >= recoveryTime_) {
                        s->setHealth(ServerHealth::HEALTHY);
                        downSince_.erase(it);
                        if (onEvent_) onEvent_(s->id(), "RECOVERED");
                    }
                } else if (chance(rng_) < failureProbability_) {
                    s->setHealth(ServerHealth::DOWN);
                    downSince_[s->id()] = now;
                    if (onEvent_) onEvent_(s->id(), "FAILED");
                }
            }
        }
    }

    std::vector<std::unique_ptr<Server>>& servers_;
    double failureProbability_;
    std::chrono::milliseconds checkInterval_;
    std::chrono::milliseconds recoveryTime_;
    std::function<void(int, const std::string&)> onEvent_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::unordered_map<int, std::chrono::steady_clock::time_point> downSince_;
    std::mt19937 rng_;
};
