#pragma once
#include <random>
#include <thread>
#include <atomic>
#include "Request.h"
#include "ThreadSafeQueue.h"

// Producer: generates a configurable number of synthetic requests with
// randomized priority/payload/processing-time, and pushes them onto the
// shared queue that worker threads consume from.
class RequestGenerator {
public:
    RequestGenerator(ThreadSafeQueue<Request>& queue, uint64_t requestCount,
                      double minProcessingMs, double maxProcessingMs,
                      size_t minPayloadBytes, size_t maxPayloadBytes)
        : queue_(queue), requestCount_(requestCount),
          procDist_(minProcessingMs, maxProcessingMs),
          payloadDist_(minPayloadBytes, maxPayloadBytes),
          priorityDist_(0, 2),
          rng_(std::random_device{}()) {}

    void run() {
        for (uint64_t i = 0; i < requestCount_; ++i) {
            Request r = Request::create(priorityDist_(rng_),
                                         payloadDist_(rng_),
                                         procDist_(rng_));
            queue_.push(std::move(r));
        }
    }

private:
    ThreadSafeQueue<Request>& queue_;
    uint64_t requestCount_;
    std::uniform_real_distribution<double> procDist_;
    std::uniform_int_distribution<size_t> payloadDist_;
    std::uniform_int_distribution<int> priorityDist_;
    std::mt19937 rng_;
};
