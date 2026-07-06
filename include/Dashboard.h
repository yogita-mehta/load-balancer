#pragma once
#include <iostream>
#include <iomanip>
#include "LoadBalancer.h"

// Renders a periodically-refreshed terminal snapshot of the LoadBalancer's
// state. Not a full curses UI (kept dependency-free), but redraws in place
// using ANSI escape codes so it reads like a live dashboard.
class Dashboard {
public:
    explicit Dashboard(LoadBalancer& lb) : lb_(lb) {}

    void render() {
        auto snap = lb_.snapshot();
        std::cout << "\033[2J\033[H"; // clear screen, move cursor home
        std::cout << "================ LOAD BALANCER DASHBOARD ================\n";
        std::cout << "Algorithm        : " << snap.algorithm << "\n";
        std::cout << "Healthy Servers  : " << snap.healthyServers << "\n";
        std::cout << "Failed Servers   : " << snap.failedServers << "\n";
        std::cout << "Active Conns     : " << snap.totalConnections << "\n";
        std::cout << "Queue Length     : " << snap.queueLength << "\n";
        std::cout << "Completed        : " << snap.completed << "\n";
        std::cout << "Dropped          : " << snap.dropped << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Avg Latency (ms) : " << snap.avgLatencyMs << "\n";
        std::cout << "----------------------------------------------------------\n";
        for (auto& s : lb_.servers()) {
            std::cout << "  Server " << s->id()
                      << " | health=" << healthStr(s->health())
                      << " | conns=" << s->activeConnections()
                      << "/" << s->capacity()
                      << " | cpu=" << std::setprecision(1) << s->cpuUtilization() << "%"
                      << " | mem=" << s->memUtilization() << "%"
                      << " | avgRT=" << std::setprecision(2) << s->avgResponseTimeMs() << "ms"
                      << " | served=" << s->totalServed()
                      << " | failed=" << s->totalFailed()
                      << "\n";
        }
        std::cout << "==========================================================\n";
        std::cout.flush();
    }

private:
    static const char* healthStr(ServerHealth h) {
        switch (h) {
            case ServerHealth::HEALTHY: return "HEALTHY";
            case ServerHealth::DEGRADED: return "DEGRADED";
            case ServerHealth::DOWN: return "DOWN";
        }
        return "?";
    }

    LoadBalancer& lb_;
};
