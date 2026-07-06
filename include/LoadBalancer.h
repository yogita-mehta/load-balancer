#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <atomic>
#include "Server.h"
#include "IStrategy.h"
#include "ThreadPool.h"
#include "ThreadSafeQueue.h"
#include "Request.h"
#include "MetricsCollector.h"
#include "CircuitBreaker.h"
#include "HealthMonitor.h"
#include "IObserver.h"
#include "Logger.h"

struct LoadBalancerConfig {
    int serverCount = 5;
    std::vector<int> serverWeights;
    std::vector<int> serverCapacities;
    int threadCount = 8;
    uint64_t requestCount = 1000;
    double failureProbability = 0.01;
    int healthCheckIntervalMs = 500;
    int recoveryTimeMs = 3000;
    int maxRetries = 3;
    int circuitBreakerThreshold = 5;
    int circuitBreakerCooldownMs = 2000;
    double minProcessingMs = 5.0;
    double maxProcessingMs = 50.0;
    size_t minPayloadBytes = 512;
    size_t maxPayloadBytes = 65536;
    size_t queueCapacity = 10000;
    std::string strategyName = "RoundRobin";
    double timeScale = 1.0; // scales simulated processing time; 0 = no sleep (max throughput)
};

// Builder Pattern: incrementally configure a LoadBalancer before construction.
class LoadBalancerBuilder {
public:
    LoadBalancerBuilder& withConfig(const LoadBalancerConfig& cfg) { cfg_ = cfg; return *this; }
    LoadBalancerBuilder& withStrategy(const std::string& name) { cfg_.strategyName = name; return *this; }
    LoadBalancerBuilder& withRequestCount(uint64_t n) { cfg_.requestCount = n; return *this; }
    LoadBalancerConfig build() const { return cfg_; }

private:
    LoadBalancerConfig cfg_;
};

// The central orchestrator: owns the server pool, the chosen scheduling
// strategy, the thread pool of workers, and coordinates fault-tolerance
// (circuit breakers + health monitor) and metrics collection.
class LoadBalancer {
public:
    LoadBalancer(LoadBalancerConfig config, std::unique_ptr<IStrategy> strategy,
                 Logger* appLog = nullptr, Logger* errLog = nullptr);

    // Runs the full simulation synchronously: spins up the request
    // generator + worker pool, processes config.requestCount requests,
    // and blocks until all are drained. Returns computed metrics summary.
    MetricsCollector::Summary runSimulation();

    void addObserver(IObserver* obs) { observers_.push_back(obs); }

    const std::vector<std::unique_ptr<Server>>& servers() const { return servers_; }
    std::string strategyName() const { return strategy_->name(); }
    MetricsCollector& metrics() { return metrics_; }

    // Snapshot used by the terminal dashboard.
    struct Snapshot {
        std::string algorithm;
        int healthyServers = 0;
        int failedServers = 0;
        int totalConnections = 0;
        size_t queueLength = 0;
        uint64_t completed = 0;
        uint64_t dropped = 0;
        double avgLatencyMs = 0;
    };
    Snapshot snapshot();

private:
    void handleRequest(Request req);
    void notifyServerEvent(int serverId, const std::string& event);

    LoadBalancerConfig config_;
    std::unique_ptr<IStrategy> strategy_;
    std::vector<std::unique_ptr<Server>> servers_;
    std::unordered_map<int, std::unique_ptr<CircuitBreaker>> breakers_;
    ThreadSafeQueue<Request> queue_;
    ThreadPool pool_;
    MetricsCollector metrics_;
    std::unique_ptr<HealthMonitor> healthMonitor_;
    std::vector<IObserver*> observers_;
    Logger* appLog_;
    Logger* errLog_;
    std::atomic<uint64_t> inFlight_{0};
};
