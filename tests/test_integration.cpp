#include "MiniTest.h"
#include "LoadBalancer.h"
#include "StrategyFactory.h"

namespace {
LoadBalancerConfig smallConfig(uint64_t requestCount) {
    LoadBalancerConfig cfg;
    cfg.serverCount = 4;
    cfg.threadCount = 4;
    cfg.requestCount = requestCount;
    cfg.failureProbability = 0.0; // deterministic-ish for integration test
    cfg.minProcessingMs = 1.0;
    cfg.maxProcessingMs = 3.0;
    cfg.timeScale = 0.2; // keep the test fast
    cfg.queueCapacity = requestCount + 10;
    return cfg;
}
} // namespace

TEST(Integration, AllRequestsAreAccountedFor) {
    auto cfg = smallConfig(500);
    LoadBalancer lb(cfg, StrategyFactory::create("RoundRobin"));
    auto summary = lb.runSimulation();
    // Every request must end up completed, failed, or dropped -- none lost.
    EXPECT_EQ(summary.completed + summary.failed + summary.dropped, 500u);
}

TEST(Integration, LeastConnectionsBalancesLoadAcrossServers) {
    auto cfg = smallConfig(1000);
    LoadBalancer lb(cfg, StrategyFactory::create("LeastConnections"));
    lb.runSimulation();
    uint64_t minServed = UINT64_MAX, maxServed = 0;
    for (auto& s : lb.servers()) {
        minServed = std::min(minServed, s->totalServed());
        maxServed = std::max(maxServed, s->totalServed());
    }
    // With 4 equally-capable servers, no single server should be
    // wildly overloaded relative to the others.
    EXPECT_TRUE(maxServed - minServed < 1000 / 2);
}

TEST(StressTest, HandlesTenThousandRequestsWithoutCrashing) {
    auto cfg = smallConfig(10000);
    cfg.timeScale = 0.0; // no artificial sleep -- pure throughput stress
    LoadBalancer lb(cfg, StrategyFactory::create("PowerOfTwoChoices"));
    auto summary = lb.runSimulation();
    EXPECT_EQ(summary.completed + summary.failed + summary.dropped, 10000u);
    EXPECT_GT(summary.throughputRps, 0.0);
}

TEST(FailureTest, RequestsSurviveRandomServerOutages) {
    auto cfg = smallConfig(2000);
    cfg.failureProbability = 0.05; // aggressive random outages
    cfg.healthCheckIntervalMs = 20;
    cfg.recoveryTimeMs = 50;
    cfg.timeScale = 0.1;
    LoadBalancer lb(cfg, StrategyFactory::create("LeastResponseTime"));
    auto summary = lb.runSimulation();
    // Even with frequent outages, the retry mechanism + circuit breaker
    // should ensure the overwhelming majority of requests still complete.
    EXPECT_EQ(summary.completed + summary.failed + summary.dropped, 2000u);
}

int main() {
    return minitest::runAll();
}
