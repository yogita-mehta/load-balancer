#pragma once
#include <cstdint>
#include <chrono>
#include <atomic>

// Represents a single unit of work flowing through the load balancer.
struct Request {
    uint64_t id;
    int priority;              // 0 = highest, higher number = lower priority
    size_t payloadBytes;
    std::chrono::steady_clock::time_point arrivalTime;
    std::chrono::steady_clock::time_point dispatchTime;   // when assigned to a server
    std::chrono::steady_clock::time_point completionTime; // when server finished it
    double processingTimeMs;   // simulated cost of serving this request
    int assignedServerId = -1;
    int retryCount = 0;
    bool failed = false;

    static std::atomic<uint64_t> nextId;

    static Request create(int priority, size_t payloadBytes, double processingTimeMs) {
        Request r;
        r.id = nextId.fetch_add(1, std::memory_order_relaxed);
        r.priority = priority;
        r.payloadBytes = payloadBytes;
        r.processingTimeMs = processingTimeMs;
        r.arrivalTime = std::chrono::steady_clock::now();
        return r;
    }

    double queueWaitMs() const {
        return std::chrono::duration<double, std::milli>(dispatchTime - arrivalTime).count();
    }

    double totalLatencyMs() const {
        return std::chrono::duration<double, std::milli>(completionTime - arrivalTime).count();
    }
};
