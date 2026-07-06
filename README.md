# Load Balancer Simulation (C++17)

A multithreaded, production-inspired load balancer simulator. Six interchangeable
scheduling algorithms (Strategy pattern), a simulated server cluster with health,
circuit breakers, automatic failure/recovery, a thread-pool based producer/consumer
pipeline, a live terminal dashboard, and an automated benchmarking harness that
writes real CSV results (no fabricated numbers).

## Build

Requires g++ (C++17) and CMake >= 3.15.

```bash
mkdir -p build && cd build
cmake ..
make -j
```

This produces:
- `build/loadbalancer` — the main executable
- `build/test_strategies`, `build/test_reliability`, `build/test_integration` — test binaries (also runnable via `ctest`)

If you don't have CMake, you can compile directly:

```bash
g++ -std=c++17 -O2 -Iinclude -pthread src/Request.cpp src/LoadBalancer.cpp src/main.cpp -o loadbalancer
```

## Run

```bash
# Live simulation with a refreshing terminal dashboard
./build/loadbalancer simulate config/config.json

# Benchmark all 6 algorithms across 100 / 1,000 / 10,000 / 100,000 requests
# (add "request_count": 1000000 in the config to also include a 1M-request pass)
./build/loadbalancer benchmark config/config.json
```

Output artifacts:
- `logs/application.log`, `logs/errors.log` — structured logs
- `reports/benchmark.csv`, `reports/metrics.csv` — per-run metrics (latency percentiles, throughput, drops, retries)

## Architecture

| Component | File(s) | Responsibility |
|---|---|---|
| `IStrategy` / `Strategies.h` | Strategy pattern | RoundRobin, WeightedRoundRobin, LeastConnections, Random, PowerOfTwoChoices, LeastResponseTime |
| `StrategyFactory` | Factory pattern | String → strategy instance |
| `LoadBalancerBuilder` | Builder pattern | Incremental config construction |
| `IObserver` | Observer pattern | Server/request event subscribers (dashboard, logging) |
| `Server` | Simulated backend | Connections, capacity, weight, simulated CPU/mem, EMA response time |
| `ThreadSafeQueue` / `ThreadPool` | Concurrency primitives | Producer/consumer request pipeline |
| `HealthMonitor` | Background thread | Random failures + automatic recovery |
| `CircuitBreaker` | Per-server | CLOSED → OPEN → HALF_OPEN → CLOSED fault isolation |
| `MetricsCollector` | Stats | avg/median/p95/p99/max latency, throughput, CSV export |
| `Dashboard` | Terminal UI | Live-refreshing ANSI snapshot |

## Testing

Tests use a small header-only assertion framework (`tests/MiniTest.h`) so the
project builds with zero external dependencies. Swap in GoogleTest in
`tests/CMakeLists.txt` if you prefer it — the test bodies use plain
`EXPECT_*` macros so migration is mechanical.

```bash
cd build && ctest --output-on-failure
```

17 tests covering strategy correctness, circuit-breaker state transitions,
metrics math, and full end-to-end integration/stress/failure scenarios (all
passing as of the last run in this repo).

## Configuration (`config/config.json`)

All of the following are hot-configurable without recompiling: `algorithm`,
`server_count`, `server_weights`, `server_capacities`, `thread_count`,
`request_count`, `failure_probability`, `health_check_interval_ms`,
`recovery_time_ms`, `max_retries`, `circuit_breaker_threshold`,
`circuit_breaker_cooldown_ms`, `min_processing_ms`/`max_processing_ms`,
`min_payload_bytes`/`max_payload_bytes`, `queue_capacity`, `time_scale`.

`time_scale` controls whether the simulator actually sleeps for the
simulated processing time (1.0 = realistic timing) or skips sleeping
(0.0 = maximum-throughput mode, useful for 100k/1M-request benchmark runs).
