#pragma once
#include "IStrategy.h"
#include <atomic>
#include <random>
#include <limits>
#include <mutex>

// ---------------------------------------------------------------------------
// Round Robin: cycles through servers in fixed order, skipping unavailable ones.
// ---------------------------------------------------------------------------
class RoundRobinStrategy : public IStrategy {
public:
    Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) override {
        if (servers.empty()) return nullptr;
        size_t n = servers.size();
        for (size_t attempts = 0; attempts < n; ++attempts) {
            size_t idx = index_.fetch_add(1, std::memory_order_relaxed) % n;
            if (servers[idx]->isAvailable()) return servers[idx].get();
        }
        return nullptr;
    }
    std::string name() const override { return "RoundRobin"; }

private:
    std::atomic<size_t> index_{0};
};

// ---------------------------------------------------------------------------
// Weighted Round Robin: servers with higher weight receive proportionally
// more requests. Uses the smooth weighted round robin algorithm (as used by
// nginx) so bursts are spread evenly rather than clumped.
// ---------------------------------------------------------------------------
class WeightedRoundRobinStrategy : public IStrategy {
public:
    Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) override {
        std::lock_guard<std::mutex> lk(mutex_);
        if (servers.empty()) return nullptr;
        if (currentWeights_.size() != servers.size()) {
            currentWeights_.assign(servers.size(), 0);
        }

        int totalWeight = 0;
        for (auto& s : servers) totalWeight += s->weight();
        if (totalWeight <= 0) return nullptr;

        int bestIdx = -1;
        for (size_t i = 0; i < servers.size(); ++i) {
            if (!servers[i]->isAvailable()) continue;
            currentWeights_[i] += servers[i]->weight();
            if (bestIdx == -1 || currentWeights_[i] > currentWeights_[bestIdx]) {
                bestIdx = static_cast<int>(i);
            }
        }
        if (bestIdx == -1) return nullptr;
        currentWeights_[bestIdx] -= totalWeight;
        return servers[bestIdx].get();
    }
    std::string name() const override { return "WeightedRoundRobin"; }

private:
    std::mutex mutex_;
    std::vector<int> currentWeights_;
};

// ---------------------------------------------------------------------------
// Least Connections: routes to the available server with fewest active
// connections. Good default for heterogeneous request durations.
// ---------------------------------------------------------------------------
class LeastConnectionsStrategy : public IStrategy {
public:
    Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) override {
        Server* best = nullptr;
        int bestConns = std::numeric_limits<int>::max();
        for (auto& s : servers) {
            if (!s->isAvailable()) continue;
            int c = s->activeConnections();
            if (c < bestConns) {
                bestConns = c;
                best = s.get();
            }
        }
        return best;
    }
    std::string name() const override { return "LeastConnections"; }
};

// ---------------------------------------------------------------------------
// Random: uniformly picks any available server. Baseline for comparison.
// ---------------------------------------------------------------------------
class RandomStrategy : public IStrategy {
public:
    RandomStrategy() : rng_(std::random_device{}()) {}

    Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) override {
        std::vector<Server*> available;
        for (auto& s : servers) if (s->isAvailable()) available.push_back(s.get());
        if (available.empty()) return nullptr;
        std::lock_guard<std::mutex> lk(mutex_);
        std::uniform_int_distribution<size_t> dist(0, available.size() - 1);
        return available[dist(rng_)];
    }
    std::string name() const override { return "Random"; }

private:
    std::mutex mutex_;
    std::mt19937 rng_;
};

// ---------------------------------------------------------------------------
// Power of Two Choices: samples two random available servers and picks the
// one with fewer active connections. Proven to dramatically reduce the
// worst-case load imbalance compared to pure random, at O(1) cost.
// ---------------------------------------------------------------------------
class PowerOfTwoChoicesStrategy : public IStrategy {
public:
    PowerOfTwoChoicesStrategy() : rng_(std::random_device{}()) {}

    Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) override {
        std::vector<Server*> available;
        for (auto& s : servers) if (s->isAvailable()) available.push_back(s.get());
        if (available.empty()) return nullptr;
        if (available.size() == 1) return available[0];

        std::lock_guard<std::mutex> lk(mutex_);
        std::uniform_int_distribution<size_t> dist(0, available.size() - 1);
        Server* a = available[dist(rng_)];
        Server* b = available[dist(rng_)];
        return (a->activeConnections() <= b->activeConnections()) ? a : b;
    }
    std::string name() const override { return "PowerOfTwoChoices"; }

private:
    std::mutex mutex_;
    std::mt19937 rng_;
};

// ---------------------------------------------------------------------------
// Least Response Time: routes to the available server with the lowest
// exponentially-weighted average historical response time. Approximates
// what real L7 load balancers (e.g. Envoy's "least request" variants) do.
// ---------------------------------------------------------------------------
class LeastResponseTimeStrategy : public IStrategy {
public:
    Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) override {
        Server* best = nullptr;
        double bestScore = std::numeric_limits<double>::max();
        for (auto& s : servers) {
            if (!s->isAvailable()) continue;
            // Combine historical latency with current queue depth so a
            // server that's fast on average but currently swamped is deprioritized.
            double score = s->avgResponseTimeMs() * (1.0 + s->activeConnections());
            if (score < bestScore) {
                bestScore = score;
                best = s.get();
            }
        }
        return best;
    }
    std::string name() const override { return "LeastResponseTime"; }
};
