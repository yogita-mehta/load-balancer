#pragma once
#include <atomic>
#include <chrono>
#include <mutex>

// Per-server circuit breaker: OPEN state stops new traffic from being routed
// to a repeatedly-failing server; after a cooldown it moves to HALF_OPEN to
// probe recovery; a success closes it again.
class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };

    CircuitBreaker(int failureThreshold = 5,
                   std::chrono::milliseconds cooldown = std::chrono::milliseconds(2000))
        : failureThreshold_(failureThreshold), cooldown_(cooldown) {}

    bool allowRequest() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (state_ == State::OPEN) {
            auto elapsed = std::chrono::steady_clock::now() - openedAt_;
            if (elapsed >= cooldown_) {
                state_ = State::HALF_OPEN;
                return true; // allow a single probe request through
            }
            return false;
        }
        return true;
    }

    void recordSuccess() {
        std::lock_guard<std::mutex> lk(mutex_);
        consecutiveFailures_ = 0;
        state_ = State::CLOSED;
    }

    void recordFailure() {
        std::lock_guard<std::mutex> lk(mutex_);
        consecutiveFailures_++;
        if (state_ == State::HALF_OPEN || consecutiveFailures_ >= failureThreshold_) {
            state_ = State::OPEN;
            openedAt_ = std::chrono::steady_clock::now();
        }
    }

    State state() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return state_;
    }

private:
    mutable std::mutex mutex_;
    State state_ = State::CLOSED;
    int consecutiveFailures_ = 0;
    int failureThreshold_;
    std::chrono::milliseconds cooldown_;
    std::chrono::steady_clock::time_point openedAt_;
};
