#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <vector>
#include "Server.h"

class LoadBalancer {
private:
    std::vector<Server> minHeap;

    int parent(int i);
    int left(int i);
    int right(int i);
    void heapifyUp(int i);
    void heapifyDown(int i);

public:
    LoadBalancer(int num_servers);
    
    void addRequest(int duration);
    void printStatus();
    void tick();

    // NEW: Simulate a catastrophic failure
    void crashRandomServer();
};

#endif
