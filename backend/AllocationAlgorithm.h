#ifndef ALLOCATION_ALGORITHM_H
#define ALLOCATION_ALGORITHM_H

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

struct Block {
    int size;
    int pid; // Process ID, -1 represents a free hole
};

class MemoryManager {
private:
    std::vector<Block> mem;
    int nextPid{5};

public:
    MemoryManager() {
        loadDefaultPreset();
    }

    void loadDefaultPreset() {
        mem = {
            {20, 1},   // P1
            {15, -1},  // Hole
            {10, 2},   // P2
            {30, -1},  // Hole
            {15, 3},   // P3
            {10, -1},  // Hole
            {8,  4},   // P4
            {12, -1}   // Hole
        };
        nextPid = 5;
    }

    const std::vector<Block>& getBlocks() const {
        return mem;
    }

    // Find suitable hole: 1 = First Fit, 2 = Best Fit, 3 = Worst Fit
    int findHole(int req, int algo) const {
        int c = -1;
        int size = static_cast<int>(mem.size());
        for (int i = 0; i < size; i++) {
            if (mem[i].pid != -1 || mem[i].size < req) continue;

            if (c == -1 || (algo == 2 && mem[i].size < mem[c].size)
                || (algo == 3 && mem[i].size > mem[c].size)) {
                c = i;
            }
            if (algo == 1) break; // First Fit
        }
        return c;
    }

    bool allocate(int req, int algo, std::string* statusOut = nullptr) {
        if (req <= 0) {
            if (statusOut) *statusOut = "Invalid process size!";
            return false;
        }

        int i = findHole(req, algo);
        if (i == -1) {
            if (statusOut) *statusOut = "Allocation failed! No suitable hole found.";
            return false;
        }

        int left = mem[i].size - req;
        int allocatedPid = nextPid++;

        if (statusOut) {
            *statusOut = "P" + std::to_string(allocatedPid) + " allocated in hole of size "
                + std::to_string(mem[i].size) + " (leftover " + std::to_string(left) + ")";
        }

        mem[i] = { req, allocatedPid };
        if (left > 0) {
            mem.insert(mem.begin() + i + 1, { left, -1 });
        }
        return true;
    }

    bool release(int pid, std::string* statusOut = nullptr) {
        bool found = false;
        for (auto& b : mem) {
            if (b.pid == pid) {
                b.pid = -1;
                found = true;
            }
        }
        if (!found) {
            if (statusOut) *statusOut = "Process P" + std::to_string(pid) + " not found!";
            return false;
        }

        coalesce();

        if (statusOut) {
            *statusOut = "Process P" + std::to_string(pid) + " released.";
        }
        return true;
    }

    void coalesce() {
        for (size_t i = 0; i + 1 < mem.size(); ) {
            if (mem[i].pid == -1 && mem[i + 1].pid == -1) {
                mem[i].size += mem[i + 1].size;
                mem.erase(mem.begin() + i + 1);
            } else {
                i++;
            }
        }
    }

    void compact() {
        std::vector<Block> temp;
        int freeSize = 0;

        for (size_t i = 0; i < mem.size(); i++) {
            if (mem[i].pid != -1)
                temp.push_back(mem[i]);
            else
                freeSize += mem[i].size;
        }

        if (freeSize > 0)
            temp.push_back({ freeSize, -1 });

        mem = temp;
    }

    void getMetrics(int& freeMem, int& largest, int& frag) const {
        freeMem = 0;
        largest = 0;
        for (const auto& b : mem) {
            if (b.pid == -1) {
                freeMem += b.size;
                if (b.size > largest) {
                    largest = b.size;
                }
            }
        }
        double f = freeMem ? (1.0 - static_cast<double>(largest) / freeMem) * 100.0 : 0.0;
        frag = static_cast<int>(std::round(f));
    }
};

#endif // ALLOCATION_ALGORITHM_H
