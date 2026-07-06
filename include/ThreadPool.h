#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <future>
#include "ThreadSafeQueue.h"

// Fixed-size worker pool. Tasks are std::function<void()> closures placed
// on a thread-safe queue; each worker thread loops pop->execute.
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    void submit(std::function<void()> task) {
        tasks_.push(std::move(task));
    }

    void shutdown() {
        if (shuttingDown_.exchange(true)) return;
        tasks_.shutdown();
        for (auto& t : workers_) if (t.joinable()) t.join();
    }

    size_t pendingTasks() const { return tasks_.size(); }
    size_t threadCount() const { return workers_.size(); }

private:
    void workerLoop() {
        while (true) {
            auto task = tasks_.pop();
            if (!task.has_value()) break; // shutdown + drained
            (*task)();
        }
    }

    std::vector<std::thread> workers_;
    ThreadSafeQueue<std::function<void()>> tasks_;
    std::atomic<bool> shuttingDown_{false};
};
