#include <iostream>
#include <algorithm>
#include <cstdlib> // For rand()
#include "../include/LoadBalancer.h"

LoadBalancer::LoadBalancer(int num_servers) {
    for (int i = 1; i <= num_servers; i++) {
        Server s(i);
        minHeap.push_back(s);
    }
}

// --- Heap Logic ---
int LoadBalancer::parent(int i) { return (i - 1) / 2; }
int LoadBalancer::left(int i) { return (2 * i) + 1; }
int LoadBalancer::right(int i) { return (2 * i) + 2; }

void LoadBalancer::heapifyUp(int i) {
    while (i > 0 && minHeap[i].current_load < minHeap[parent(i)].current_load) {
        std::swap(minHeap[i], minHeap[parent(i)]);
        i = parent(i);
    }
}

void LoadBalancer::heapifyDown(int i) {
    int minIndex = i;
    int l = left(i);
    int r = right(i);

    if (l < minHeap.size() && minHeap[l].current_load < minHeap[minIndex].current_load)
        minIndex = l;
    
    if (r < minHeap.size() && minHeap[r].current_load < minHeap[minIndex].current_load)
        minIndex = r;

    if (i != minIndex) {
        std::swap(minHeap[i], minHeap[minIndex]);
        heapifyDown(minIndex);
    }
}

// --- Core Actions ---

void LoadBalancer::addRequest(int duration) {
    if (minHeap.empty()) return;

    // Check if the best server is actually alive
    if (!minHeap[0].is_active) {
        std::cout << "[ERROR] All servers are down! Request dropped." << std::endl;
        return;
    }

    minHeap[0].current_load += duration;
    
    std::cout << ">> NEW REQUEST (" << duration << "s) -> Assigned to Server " 
              << minHeap[0].id << std::endl;

    heapifyDown(0);
}

void LoadBalancer::printStatus() {
    std::cout << "   [Cluster]: ";
    for (const auto& s : minHeap) {
        if (s.is_active) {
            std::cout << "S" << s.id << ":" << s.current_load << " ";
        } else {
            std::cout << "S" << s.id << ":[DEAD] ";
        }
    }
    std::cout << std::endl;
}

void LoadBalancer::tick() {
    for (int i = 0; i < minHeap.size(); i++) {
        // Only decrease load if server is active and has work
        if (minHeap[i].is_active && minHeap[i].current_load > 0) {
            minHeap[i].current_load--;
        }
    }

    // Rebuild heap to maintain order
    for (int i = (minHeap.size() / 2) - 1; i >= 0; i--) {
        heapifyDown(i);
    }
}

// --- NEW: Fault Tolerance ---
void LoadBalancer::crashRandomServer() {
    if (minHeap.empty()) return;

    // Simple hack: Pick the first active server we find to kill (for demo purposes)
    // In a real app, we would pick a random index.
    int victimIndex = -1;
    for (int i=0; i<minHeap.size(); i++) {
        if (minHeap[i].is_active) {
            victimIndex = i;
            break;
        }
    }

    if (victimIndex != -1) {
        Server& victim = minHeap[victimIndex];
        int lostLoad = victim.current_load;
        
        std::cout << "\n[ALERT] SERVER " << victim.id << " CRASHED! (Lost Load: " << lostLoad << ")" << std::endl;
        
        victim.is_active = false;
        victim.current_load = 9999; // Set to infinity so it sinks to bottom
        
        // Push the dead server to the bottom
        heapifyDown(victimIndex);

        // Redistribute the lost work to survivors
        if (lostLoad > 0) {
            std::cout << "[RECOVERY] Redistributing lost load to healthy servers..." << std::endl;
            addRequest(lostLoad); // Re-assign the total work to someone else
        }
    }
}
