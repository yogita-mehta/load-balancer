# High-Performance Load Balancer Simulation 🚀

A C++ simulation of a **Least-Connections Load Balancer** that efficiently distributes concurrent traffic across a server cluster using a **Min-Heap** architecture.

## 🔥 Key Features
- **O(1) Server Selection:** Uses a custom Min-Heap to find the least-loaded server instantly.
- **Fault Tolerance:** Simulates server crashes and automatically redistributes lost payloads to healthy nodes.
- **Real-Time Simulation:** "Ticks" every second to process tasks and free up resources dynamically.
- **Visual Logging:** Generates a clean, professional terminal dashboard to monitor cluster health.

## 🛠️ Tech Stack
- **Language:** C++ (Standard 17)
- **Concepts:** Priority Queues, Heap Data Structures, System Design, Fault Recovery.

## ⚡ How to Run
1. Compile:
g++ src/main.cpp src/LoadBalancer.cpp -o app

2. Run:
./app
