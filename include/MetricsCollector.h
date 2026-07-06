#pragma once
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <string>
#include <chrono>
#include <cmath>
#include <cstdint>

// Thread-safe collector for per-request latency samples plus aggregate
// counters. Produces percentile statistics and CSV reports.
class MetricsCollector {
public:
    void recordCompletion(double totalLatencyMs, double queueWaitMs, bool success) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (success) {
            latenciesMs_.push_back(totalLatencyMs);
            queueWaitsMs_.push_back(queueWaitMs);
            completed_++;
        } else {
            failed_++;
        }
    }

    void recordDropped() { dropped_.fetch_add(1, std::memory_order_relaxed); }
    void recordRetry() { retries_.fetch_add(1, std::memory_order_relaxed); }

    void markStart() { startTime_ = std::chrono::steady_clock::now(); }
    void markEnd() { endTime_ = std::chrono::steady_clock::now(); }

    struct Summary {
        uint64_t completed = 0;
        uint64_t failed = 0;
        uint64_t dropped = 0;
        uint64_t retries = 0;
        double avgLatencyMs = 0;
        double medianLatencyMs = 0;
        double p95LatencyMs = 0;
        double p99LatencyMs = 0;
        double maxLatencyMs = 0;
        double avgQueueWaitMs = 0;
        double throughputRps = 0;
        double wallClockSeconds = 0;
    };

    Summary computeSummary() {
        std::lock_guard<std::mutex> lk(mutex_);
        Summary s;
        s.completed = completed_;
        s.failed = failed_;
        s.dropped = dropped_.load();
        s.retries = retries_.load();

        if (!latenciesMs_.empty()) {
            std::vector<double> sorted = latenciesMs_;
            std::sort(sorted.begin(), sorted.end());
            double sum = 0;
            for (double v : sorted) sum += v;
            s.avgLatencyMs = sum / sorted.size();
            s.medianLatencyMs = percentile(sorted, 0.50);
            s.p95LatencyMs = percentile(sorted, 0.95);
            s.p99LatencyMs = percentile(sorted, 0.99);
            s.maxLatencyMs = sorted.back();

            double qsum = 0;
            for (double v : queueWaitsMs_) qsum += v;
            s.avgQueueWaitMs = qsum / queueWaitsMs_.size();
        }

        s.wallClockSeconds = std::chrono::duration<double>(endTime_ - startTime_).count();
        if (s.wallClockSeconds > 0) {
            s.throughputRps = static_cast<double>(s.completed) / s.wallClockSeconds;
        }
        return s;
    }

    // Appends a row to a shared benchmark CSV file (creates header if new file).
    static void appendBenchmarkCsv(const std::string& path,
                                    const std::string& algorithm,
                                    uint64_t requestCount,
                                    size_t serverCount,
                                    size_t threadCount,
                                    const Summary& s) {
        bool exists = std::ifstream(path).good();
        std::ofstream out(path, std::ios::app);
        if (!exists) {
            out << "algorithm,request_count,server_count,thread_count,completed,failed,dropped,retries,"
                   "avg_latency_ms,median_latency_ms,p95_latency_ms,p99_latency_ms,max_latency_ms,"
                   "avg_queue_wait_ms,throughput_rps,wall_clock_seconds\n";
        }
        out << algorithm << ',' << requestCount << ',' << serverCount << ',' << threadCount << ','
            << s.completed << ',' << s.failed << ',' << s.dropped << ',' << s.retries << ','
            << s.avgLatencyMs << ',' << s.medianLatencyMs << ',' << s.p95LatencyMs << ','
            << s.p99LatencyMs << ',' << s.maxLatencyMs << ',' << s.avgQueueWaitMs << ','
            << s.throughputRps << ',' << s.wallClockSeconds << '\n';
    }

private:
    static double percentile(const std::vector<double>& sorted, double p) {
        if (sorted.empty()) return 0.0;
        double idx = p * (sorted.size() - 1);
        size_t lo = static_cast<size_t>(std::floor(idx));
        size_t hi = static_cast<size_t>(std::ceil(idx));
        if (lo == hi) return sorted[lo];
        double frac = idx - lo;
        return sorted[lo] * (1 - frac) + sorted[hi] * frac;
    }

    std::mutex mutex_;
    std::vector<double> latenciesMs_;
    std::vector<double> queueWaitsMs_;
    uint64_t completed_ = 0;
    uint64_t failed_ = 0;
    std::atomic<uint64_t> dropped_{0};
    std::atomic<uint64_t> retries_{0};
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point endTime_;
};
