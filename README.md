# Load Balancer Simulation (C++17)

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)

A high-performance, multithreaded simulation of a production-grade Load Balancer. This project demonstrates advanced software engineering principles including the **Strategy Pattern**, **Circuit Breakers**, **Health Monitoring**, and **Thread Pool** architectures.

---

## 🚀 Features

- **6 Scheduling Algorithms**: Round Robin, Weighted Round Robin, Least Connections, Random, Power of Two Choices, and Least Response Time.
- **Fault Tolerance**: Per-server Circuit Breakers and background Health Monitors with automatic failure/recovery simulation.
- **Concurrency**: Lock-free/Thread-safe request pipeline using a custom Thread Pool.
- **Real-time Monitoring**: Live terminal-based ANSI dashboard tracking server health and throughput.
- **Scientific Benchhousing**: Automated harness providing latency percentiles (p50, p95, p99) and throughput metrics exported to CSV.

---

## 🛠️ Architecture

| Component | Responsibility | Pattern |
|---|---|---|
| **IStrategy** | Interchangeable scheduling logic | Strategy |
| **StrategyFactory** | Dynamic strategy instantiation | Factory |
| **LoadBalancerBuilder** | Fluent configuration setup | Builder |
| **HealthMonitor** | Periodic server health verification | Observer |
| **CircuitBreaker** | Fault isolation and recovery | State |
| **MetricsCollector** | Latency and throughput analysis | Registry |

---

## 🏗️ Getting Started

### Prerequisites
- GCC / G++ (C++17 compatible)
- CMake >= 3.15
- Pthread support (standard on Linux/macOS)

### Build Instructions
```bash
# Clone the repository (if you haven't already)
git clone https://github.com/yogita-mehta/load-balancer.git
cd load-balancer

# Build the project
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run Simulation
```bash
# Standard simulation mode
./loadbalancer simulate ../config/config.json

# Benchmark mode (Performance analysis)
./loadbalancer benchmark ../config/config.json
```

---

## 🧪 Testing

The project includes a comprehensive test suite covering strategy correctness, reliability, and integration.

```bash
cd build
ctest --output-on-failure
```

---

## 🐳 Docker Deployment

To run the simulation without installing local dependencies:

```bash
# Build Docker image
docker build -t loadbalancer-sim .

# Run simulation
docker run -it loadbalancer-sim simulate config/config.json
```

---

## 📊 Configuration (`config/config.json`)

Fine-tune the simulation by modifying the JSON configuration:
- `algorithm`: Strategy to use (e.g., `LeastConnections`).
- `server_count`: Number of backend nodes.
- `failure_probability`: Simulated error rate.
- `time_scale`: Speed of processing (1.0 = real-time, 0.0 = max-throughput).

---

## 📄 License
Distributed under the MIT License. See `LICENSE` for more information.

---

