#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include "LoadBalancer.h"
#include "StrategyFactory.h"
#include "JsonConfig.h"
#include "Logger.h"
#include "Dashboard.h"

namespace {

LoadBalancerConfig configFromJson(const JsonConfig& j) {
    LoadBalancerConfig cfg;
    cfg.serverCount = j.getInt("server_count", cfg.serverCount);
    cfg.serverWeights = j.getIntArray("server_weights");
    cfg.serverCapacities = j.getIntArray("server_capacities");
    cfg.threadCount = j.getInt("thread_count", cfg.threadCount);
    cfg.requestCount = static_cast<uint64_t>(j.getNumber("request_count", static_cast<double>(cfg.requestCount)));
    cfg.failureProbability = j.getNumber("failure_probability", cfg.failureProbability);
    cfg.healthCheckIntervalMs = j.getInt("health_check_interval_ms", cfg.healthCheckIntervalMs);
    cfg.recoveryTimeMs = j.getInt("recovery_time_ms", cfg.recoveryTimeMs);
    cfg.maxRetries = j.getInt("max_retries", cfg.maxRetries);
    cfg.circuitBreakerThreshold = j.getInt("circuit_breaker_threshold", cfg.circuitBreakerThreshold);
    cfg.circuitBreakerCooldownMs = j.getInt("circuit_breaker_cooldown_ms", cfg.circuitBreakerCooldownMs);
    cfg.minProcessingMs = j.getNumber("min_processing_ms", cfg.minProcessingMs);
    cfg.maxProcessingMs = j.getNumber("max_processing_ms", cfg.maxProcessingMs);
    cfg.minPayloadBytes = static_cast<size_t>(j.getNumber("min_payload_bytes", static_cast<double>(cfg.minPayloadBytes)));
    cfg.maxPayloadBytes = static_cast<size_t>(j.getNumber("max_payload_bytes", static_cast<double>(cfg.maxPayloadBytes)));
    cfg.queueCapacity = static_cast<size_t>(j.getNumber("queue_capacity", static_cast<double>(cfg.queueCapacity)));
    cfg.strategyName = j.getString("algorithm", cfg.strategyName);
    cfg.timeScale = j.getNumber("time_scale", cfg.timeScale);
    return cfg;
}

void printUsage(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " simulate [config.json]     Run one simulation with a live dashboard\n"
              << "  " << prog << " benchmark [config.json]    Run every algorithm across multiple request counts,\n"
              << "                                          writing reports/benchmark.csv\n";
}

void runSimulate(const LoadBalancerConfig& baseCfg) {
    Logger appLog("logs/application.log");
    Logger errLog("logs/errors.log");

    LoadBalancerConfig cfg = baseCfg;
    auto strategy = StrategyFactory::create(cfg.strategyName);
    std::cout << "Starting simulation | algorithm=" << cfg.strategyName
              << " | servers=" << cfg.serverCount
              << " | threads=" << cfg.threadCount
              << " | requests=" << cfg.requestCount << "\n";

    LoadBalancer lb(cfg, std::move(strategy), &appLog, &errLog);

    std::atomic<bool> done{false};
    std::thread dashboardThread([&] {
        Dashboard dash(lb);
        while (!done.load()) {
            dash.render();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        dash.render(); // final frame
    });

    auto summary = lb.runSimulation();
    done.store(true);
    dashboardThread.join();

    std::cout << "\n================ FINAL SUMMARY ================\n";
    std::cout << "Completed         : " << summary.completed << "\n";
    std::cout << "Failed            : " << summary.failed << "\n";
    std::cout << "Dropped           : " << summary.dropped << "\n";
    std::cout << "Retries           : " << summary.retries << "\n";
    std::cout << "Avg Latency (ms)  : " << summary.avgLatencyMs << "\n";
    std::cout << "Median Latency    : " << summary.medianLatencyMs << "\n";
    std::cout << "P95 Latency       : " << summary.p95LatencyMs << "\n";
    std::cout << "P99 Latency       : " << summary.p99LatencyMs << "\n";
    std::cout << "Max Latency       : " << summary.maxLatencyMs << "\n";
    std::cout << "Throughput (rps)  : " << summary.throughputRps << "\n";
    std::cout << "Wall clock (s)    : " << summary.wallClockSeconds << "\n";

    MetricsCollector::appendBenchmarkCsv("reports/metrics.csv", cfg.strategyName,
                                          cfg.requestCount, cfg.serverCount,
                                          cfg.threadCount, summary);
}

void runBenchmark(const LoadBalancerConfig& baseCfg) {
    Logger appLog("logs/application.log");
    Logger errLog("logs/errors.log");

    std::vector<uint64_t> requestCounts = {100, 1000, 10000, 100000};
    if (baseCfg.requestCount >= 1000000) requestCounts.push_back(1000000);

    for (const auto& algoName : StrategyFactory::allStrategyNames()) {
        for (uint64_t n : requestCounts) {
            LoadBalancerConfig cfg = baseCfg;
            cfg.strategyName = algoName;
            cfg.requestCount = n;
            // Skip real sleeping for very large runs so 100k/1M complete quickly;
            // latency is still measured from actual queueing + dispatch overhead.
            cfg.timeScale = (n >= 10000) ? 0.0 : 1.0;

            auto strategy = StrategyFactory::create(cfg.strategyName);
            LoadBalancer lb(cfg, std::move(strategy), &appLog, &errLog);

            std::cout << "Benchmarking " << algoName << " with " << n << " requests... ";
            std::cout.flush();
            auto summary = lb.runSimulation();
            std::cout << "done (" << summary.throughputRps << " req/s, p99="
                      << summary.p99LatencyMs << "ms)\n";

            MetricsCollector::appendBenchmarkCsv("reports/benchmark.csv", algoName, n,
                                                  cfg.serverCount, cfg.threadCount, summary);
        }
    }
    std::cout << "\nBenchmark complete. Results written to reports/benchmark.csv\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "simulate";
    std::string configPath = argc > 2 ? argv[2] : "config/config.json";

    LoadBalancerConfig cfg;
    try {
        JsonConfig json = JsonConfig::loadFromFile(configPath);
        cfg = configFromJson(json);
    } catch (const std::exception& e) {
        std::cerr << "Warning: " << e.what() << " -- using built-in defaults.\n";
    }

    if (mode == "simulate") {
        runSimulate(cfg);
    } else if (mode == "benchmark") {
        runBenchmark(cfg);
    } else {
        printUsage(argv[0]);
        return 1;
    }
    return 0;
}
