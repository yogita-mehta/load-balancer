#pragma once
#include <memory>
#include <string>
#include <stdexcept>
#include "Strategies.h"

// Factory Pattern: translates a config string into a concrete IStrategy,
// so LoadBalancer / main never need to know about concrete strategy classes.
class StrategyFactory {
public:
    static std::unique_ptr<IStrategy> create(const std::string& name) {
        if (name == "RoundRobin") return std::make_unique<RoundRobinStrategy>();
        if (name == "LeastConnections") return std::make_unique<LeastConnectionsStrategy>();
        if (name == "WeightedRoundRobin") return std::make_unique<WeightedRoundRobinStrategy>();
        if (name == "Random") return std::make_unique<RandomStrategy>();
        if (name == "PowerOfTwoChoices") return std::make_unique<PowerOfTwoChoicesStrategy>();
        if (name == "LeastResponseTime") return std::make_unique<LeastResponseTimeStrategy>();
        throw std::invalid_argument("Unknown load balancing strategy: " + name);
    }

    static std::vector<std::string> allStrategyNames() {
        return {"RoundRobin", "LeastConnections", "WeightedRoundRobin",
                "Random", "PowerOfTwoChoices", "LeastResponseTime"};
    }
};
