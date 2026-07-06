#include "MiniTest.h"
#include "CircuitBreaker.h"
#include "MetricsCollector.h"
#include <thread>
#include <chrono>

TEST(CircuitBreaker, OpensAfterThresholdFailures) {
    CircuitBreaker cb(3, std::chrono::milliseconds(50));
    EXPECT_TRUE(cb.allowRequest());
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_TRUE(cb.allowRequest()); // still closed, below threshold
    cb.recordFailure();
    EXPECT_TRUE(cb.state() == CircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CircuitBreaker, HalfOpensAfterCooldownAndClosesOnSuccess) {
    CircuitBreaker cb(1, std::chrono::milliseconds(20));
    cb.recordFailure(); // opens immediately (threshold = 1)
    EXPECT_FALSE(cb.allowRequest());
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    EXPECT_TRUE(cb.allowRequest()); // half-open probe allowed
    cb.recordSuccess();
    EXPECT_TRUE(cb.state() == CircuitBreaker::State::CLOSED);
}

TEST(MetricsCollector, ComputesPercentilesCorrectly) {
    MetricsCollector m;
    m.markStart();
    for (int i = 1; i <= 100; ++i) {
        m.recordCompletion(static_cast<double>(i), 0.0, true);
    }
    m.markEnd();
    auto s = m.computeSummary();
    EXPECT_EQ(s.completed, 100u);
    EXPECT_EQ(s.maxLatencyMs, 100.0);
    EXPECT_TRUE(s.p99LatencyMs >= 98.0);
    EXPECT_TRUE(s.medianLatencyMs >= 49.0 && s.medianLatencyMs <= 51.0);
}

TEST(MetricsCollector, TracksDroppedAndFailedSeparately) {
    MetricsCollector m;
    m.markStart();
    m.recordCompletion(10.0, 1.0, true);
    m.recordCompletion(20.0, 1.0, false); // failed
    m.recordDropped();
    m.recordDropped();
    m.markEnd();
    auto s = m.computeSummary();
    EXPECT_EQ(s.completed, 1u);
    EXPECT_EQ(s.failed, 1u);
    EXPECT_EQ(s.dropped, 2u);
}

int main() {
    return minitest::runAll();
}
