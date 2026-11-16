#ifndef PROCESS_MANAGEMENT_ALGORITHM_H
#define PROCESS_MANAGEMENT_ALGORITHM_H

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

// Original processManagement class from Process Management Algorithm.cpp
class processManagement {
public:
    struct Process {
        std::string name;
        int size;
        bool isRunning;
        std::chrono::steady_clock::time_point lastActivity;
    };

    std::vector<Process> processes;

    void addProcess(const std::string& name, int size) {
        if (size <= 0) return;
        Process p;
        p.name = name;
        p.size = size;
        p.isRunning = true;
        p.lastActivity = std::chrono::steady_clock::now();
        processes.push_back(p);
    }

    void updateActivity(const std::string& processName) {
        for (auto& p : processes) {
            if (p.name == processName && p.isRunning) {
                p.lastActivity = std::chrono::steady_clock::now();
                return;
            }
        }
    }

    void terminateProcess(const std::string& processName) {
        for (auto& p : processes) {
            if (p.name == processName && p.isRunning) {
                p.isRunning = false;
                return;
            }
        }
    }

    void inactivityCleanup(int idleMinutes = 30) {
        auto currentTime = std::chrono::steady_clock::now();
        for (auto& p : processes) {
            if (p.isRunning) {
                auto idleTime = currentTime - p.lastActivity;
                if (idleTime > std::chrono::minutes(idleMinutes)) {
                    p.isRunning = false;
                }
            }
        }
    }
};

#endif // PROCESS_MANAGEMENT_ALGORITHM_H
