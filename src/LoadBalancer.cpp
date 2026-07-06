#include "LoadBalancer.h"
#include "RequestGenerator.h"
#include <thread>
#include <random>
#include <chrono>
#include <sstream>

LoadBalancer::LoadBalancer(LoadBalancerConfig config, std::unique_ptr<IStrategy> strategy,
                            Logger* appLog, Logger* errLog)
    : config_(std::move(config)),
      strategy_(std::move(strategy)),
      queue_(config_.queueCapacity),
      pool_(config_.threadCount),
      appLog_(appLog),
      errLog_(errLog) {

    for (int i = 0; i < config_.serverCount; ++i) {
        int weight = (i < static_cast<int>(config_.serverWeights.size()))
                         ? config_.serverWeights[i] : 1;
        int capacity = (i < static_cast<int>(config_.serverCapacities.size()))
                           ? config_.serverCapacities[i] : 100;
        servers_.push_back(std::make_unique<Server>(i, weight, capacity));
        breakers_[i] = std::make_unique<CircuitBreaker>(
            config_.circuitBreakerThreshold,
            std::chrono::milliseconds(config_.circuitBreakerCooldownMs));
    }

    healthMonitor_ = std::make_unique<HealthMonitor>(
        servers_, config_.failureProbability,
        std::chrono::milliseconds(config_.healthCheckIntervalMs),
        std::chrono::milliseconds(config_.recoveryTimeMs),
        [this](int id, const std::string& evt) { notifyServerEvent(id, evt); });
}

void LoadBalancer::notifyServerEvent(int serverId, const std::string& event) {
    if (appLog_) {
        std::ostringstream oss;
        oss << "Server " << serverId << " " << event;
        appLog_->info(oss.str());
    }
    for (auto* obs : observers_) obs->onServerEvent(serverId, event);
}

void LoadBalancer::handleRequest(Request req) {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> failChance(0.0, 1.0);

    Server* chosen = nullptr;
    for (int attempt = 0; attempt <= config_.maxRetries; ++attempt) {
        chosen = strategy_->pickServer(servers_);
        if (chosen != nullptr) {
            auto& breaker = breakers_[chosen->id()];
            if (breaker->allowRequest()) break;
            chosen = nullptr; // circuit open on the only candidate this round; retry
        }
        if (attempt < config_.maxRetries) {
            metrics_.recordRetry();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    if (chosen == nullptr) {
        metrics_.recordDropped();
        if (errLog_) errLog_->error("Request " + std::to_string(req.id) + " dropped: no available server");
        inFlight_.fetch_sub(1, std::memory_order_relaxed);
        for (auto* obs : observers_) obs->onRequestCompleted(req.id, false);
        return;
    }

    req.dispatchTime = std::chrono::steady_clock::now();
    req.assignedServerId = chosen->id();
    chosen->beginRequest();

    // Simulate the server doing work. timeScale == 0 skips sleeping entirely
    // (useful for maximum-throughput benchmark runs at high request counts).
    if (config_.timeScale > 0.0) {
        std::this_thread::sleep_for(
            std::chrono::duration<double, std::milli>(req.processingTimeMs * config_.timeScale));
    }

    // Small independent failure chance simulates transient server-side errors
    // (distinct from full outages, which HealthMonitor governs).
    bool failed = failChance(rng) < 0.02;
    req.completionTime = std::chrono::steady_clock::now();
    req.failed = failed;

    double latency = req.totalLatencyMs();
    chosen->endRequest(latency, failed);

    auto& breaker = breakers_[chosen->id()];
    if (failed) breaker->recordFailure(); else breaker->recordSuccess();

    if (failed && req.retryCount < config_.maxRetries) {
        req.retryCount++;
        metrics_.recordRetry();
        pool_.submit([this, req]() mutable { handleRequest(req); });
        return; // don't decrement inFlight_ yet; the retry owns completion
    }

    metrics_.recordCompletion(latency, req.queueWaitMs(), !failed);
    if (failed && errLog_) {
        errLog_->error("Request " + std::to_string(req.id) + " failed permanently after retries");
    }
    for (auto* obs : observers_) obs->onRequestCompleted(req.id, !failed);
    inFlight_.fetch_sub(1, std::memory_order_relaxed);
}

MetricsCollector::Summary LoadBalancer::runSimulation() {
    metrics_.markStart();
    healthMonitor_->start();

    inFlight_.store(static_cast<uint64_t>(config_.requestCount));

    RequestGenerator generator(queue_, config_.requestCount,
                                config_.minProcessingMs, config_.maxProcessingMs,
                                config_.minPayloadBytes, config_.maxPayloadBytes);
    std::thread producerThread([&generator] { generator.run(); });

    // Dispatcher: pop each generated request off the shared queue and hand
    // it to the thread pool for execution against the chosen server.
    std::thread dispatcherThread([this] {
        for (uint64_t i = 0; i < config_.requestCount; ++i) {
            auto reqOpt = queue_.pop();
            if (!reqOpt.has_value()) break;
            Request r = std::move(*reqOpt);
            pool_.submit([this, r]() mutable { handleRequest(r); });
        }
    });

    producerThread.join();
    dispatcherThread.join();

    // Wait for all in-flight (and their retries) to finish.
    while (inFlight_.load(std::memory_order_relaxed) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    healthMonitor_->stop();
    metrics_.markEnd();
    return metrics_.computeSummary();
}

LoadBalancer::Snapshot LoadBalancer::snapshot() {
    Snapshot snap;
    snap.algorithm = strategy_->name();
    snap.queueLength = queue_.size();
    for (auto& s : servers_) {
        if (s->health() == ServerHealth::DOWN) snap.failedServers++;
        else snap.healthyServers++;
        snap.totalConnections += s->activeConnections();
    }
    auto summary = metrics_.computeSummary();
    snap.completed = summary.completed;
    snap.dropped = summary.dropped;
    snap.avgLatencyMs = summary.avgLatencyMs;
    return snap;
}
