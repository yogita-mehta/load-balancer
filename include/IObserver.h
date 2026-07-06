#pragma once
#include <string>

// Observer Pattern: components (dashboard, logger, alerting) subscribe to
// load-balancer lifecycle events without the LoadBalancer needing to know
// about their concrete types.
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void onServerEvent(int serverId, const std::string& eventType) = 0;
    virtual void onRequestCompleted(uint64_t requestId, bool success) = 0;
};
