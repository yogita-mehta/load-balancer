#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <ctime> // For random seed
#include "../include/LoadBalancer.h"

#ifdef _WIN32
#include <windows.h>
#define sleep_sec(x) Sleep(x * 1000)
#else
#include <unistd.h>
#define sleep_sec(x) sleep(x)
#endif

int main() {
    srand(time(0)); // Seed random number generator

    std::cout << "========================================" << std::endl;
    std::cout << "   HIGH-PERFORMANCE LOAD BALANCER v2.0  " << std::endl;
    std::cout << "   (With Fault Tolerance & Recovery)    " << std::endl;
    std::cout << "========================================" << std::endl;
    
    LoadBalancer lb(5);
    int time = 0;
    int max_time = 20;

    std::cout << "[INFO] Simulation Started...\n" << std::endl;

    while (time <= max_time) {
        
        // Traffic Logic
        if (time == 2) {
            std::cout << "[TRAFFIC] Surge detected!" << std::endl;
            lb.addRequest(10); lb.addRequest(12); lb.addRequest(8);
        }

        if (time == 7) lb.addRequest(20);

        if (time % 3 == 0 && time > 0) lb.addRequest(4);

        // --- THE CRASH EVENT ---
        if (time == 10) {
            lb.crashRandomServer();
        }

        std::cout << "T+" << std::setw(2) << time << "s | ";
        lb.printStatus();
        
        lb.tick();
        
        time++;
        sleep_sec(1);
    }

    std::cout << "\n[INFO] Simulation Terminated." << std::endl;
    return 0;
}
