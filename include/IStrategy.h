#pragma once
#include <vector>
#include <memory>
#include "Server.h"

// Strategy Pattern: every scheduling algorithm implements this interface,
// so the LoadBalancer can swap algorithms at runtime without any changes
// to its own logic (Open/Closed Principle).
class IStrategy {
public:
    virtual ~IStrategy() = default;

    // Given the current server pool, pick one server to receive the next
    // request. Returns nullptr if no server is available.
    virtual Server* pickServer(const std::vector<std::unique_ptr<Server>>& servers) = 0;

    virtual std::string name() const = 0;
};
