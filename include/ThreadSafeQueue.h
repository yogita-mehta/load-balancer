#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

// Bounded, thread-safe FIFO queue used as the hand-off point between the
// producer (RequestGenerator) and the consumer (ThreadPool workers).
template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t maxSize = 0) : maxSize_(maxSize) {}

    // Blocks if the queue is full (when maxSize_ > 0). Returns false if the
    // queue has been shut down while waiting.
    bool push(T item) {
        std::unique_lock<std::mutex> lk(mutex_);
        if (maxSize_ > 0) {
            notFull_.wait(lk, [this] { return queue_.size() < maxSize_ || shutdown_; });
        }
        if (shutdown_) return false;
        queue_.push(std::move(item));
        lk.unlock();
        notEmpty_.notify_one();
        return true;
    }

    // Blocks until an item is available or the queue is shut down and empty.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(mutex_);
        notEmpty_.wait(lk, [this] { return !queue_.empty() || shutdown_; });
        if (queue_.empty()) return std::nullopt; // shut down and drained
        T item = std::move(queue_.front());
        queue_.pop();
        lk.unlock();
        notFull_.notify_one();
        return item;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.size();
    }

    void shutdown() {
        std::lock_guard<std::mutex> lk(mutex_);
        shutdown_ = true;
        notEmpty_.notify_all();
        notFull_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    std::queue<T> queue_;
    size_t maxSize_;
    bool shutdown_ = false;
};
