#ifndef MEMORY_MANAGEMENT_ALGORITHM_H
#define MEMORY_MANAGEMENT_ALGORITHM_H

#include <iostream>
#include <vector>
#include <string>
using namespace std;

class memoryManger {

public:

    struct Block {
        string name;
        int size;
        bool isFree;
        bool isReachable;

        vector<string> references;   // CHANGED: names instead of Block*
    };

    vector<Block> memory;
    vector<string> roots;            // CHANGED: names instead of Block*


    void coalesce() {

        for (int i = 0; i + 1 < memory.size(); ) {
            if (memory[i].isFree && memory[i + 1].isFree) {
                memory[i].size += memory[i + 1].size;
                memory.erase(memory.begin() + i + 1);
            }
            else
                i++;
        }

    }

    void compact() {

        vector<Block> temp;
        int freeSize = 0;

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].isFree == false)
                temp.push_back(memory[i]);
            else
                freeSize += memory[i].size;
        }

        if (freeSize > 0)
            temp.push_back({ "", freeSize, true, false });

        memory.clear();

        for (int i = 0; i < temp.size(); i++) {
            memory.push_back(temp[i]);
        }

    }

    void freeMemory(string processName) {

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].name == processName && memory[i].isFree == false)
            {
                memory[i].isFree = true;
                memory[i].name = "";
                memory[i].references.clear();            // ADDED
            }
        }

        // ADDED: remove every root/reference that still points to the freed process
        for (int i = 0; i < roots.size(); ) {
            if (roots[i] == processName)
                roots.erase(roots.begin() + i);
            else
                i++;
        }
        for (int i = 0; i < memory.size(); i++) {
            for (int j = 0; j < memory[i].references.size(); ) {
                if (memory[i].references[j] == processName)
                    memory[i].references.erase(memory[i].references.begin() + j);
                else
                    j++;
            }
        }

        coalesce();

    }

    void addRoot(string processName) {

        for (int i = 0; i < roots.size(); i++) {
            if (roots[i] == processName) {                // CHANGED
                cout << "Root already exists!" << endl;
                return;
            }
        }

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].name == processName && memory[i].isFree == false) {
                roots.push_back(processName);             // CHANGED
                cout << "Root added successfully!" << endl;
                return;
            }
        }

        cout << "Process not found in memory!" << endl;
    }



    void removeRoot(string processName) {

        for (int i = 0; i < roots.size(); i++) {
            if (roots[i] == processName) {                // CHANGED
                roots.erase(roots.begin() + i);
                cout << "Root removed successfully!" << endl;
                return;
            }
        }

        cout << "Root not found!" << endl;

    }

    void resetMarks() {

        for (int i = 0; i < memory.size(); i++) {
            memory[i].isReachable = false;
        }

    }

    // ADDED: name -> block lookup (only used briefly, never stored)
    Block* findBlock(string name) {

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].name == name && !memory[i].isFree)
                return &memory[i];
        }
        return nullptr;

    }

    void addReference(string from, string to) {

        Block* source = nullptr;
        Block* target = nullptr;

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].name == from && !memory[i].isFree)
                source = &memory[i];
            if (memory[i].name == to && !memory[i].isFree)
                target = &memory[i];
        }

        if (source == nullptr || target == nullptr) {
            cout << "Process not found!" << endl;
            return;
        }

        for (int i = 0; i < source->references.size(); i++) {
            if (source->references[i] == to) {            // CHANGED
                cout << "Reference already exists!" << endl;
                return;
            }
        }

        source->references.push_back(to);                 // CHANGED

        cout << "Reference added successfully!" << endl;

    }

    void removeReference(string from, string to) {

        Block* source = nullptr;
        Block* target = nullptr;

        for (int i = 0; i < memory.size(); i++) {
            if (memory[i].name == from && !memory[i].isFree)
                source = &memory[i];
            if (memory[i].name == to && !memory[i].isFree)
                target = &memory[i];
        }

        if (source == nullptr || target == nullptr) {
            cout << "Process not found!" << endl;
            return;
        }

        for (int i = 0; i < source->references.size(); i++) {
            if (source->references[i] == to) {            // CHANGED
                source->references.erase(source->references.begin() + i);
                cout << "Reference removed successfully!" << endl;
                return;
            }
        }

        cout << "Reference not found!" << endl;

    }

    void mark(Block* current) {

        if (current == nullptr || current->isFree || current->isReachable)
            return;

        current->isReachable = true;

        for (int i = 0; i < current->references.size(); i++) {
            mark(findBlock(current->references[i]));      // CHANGED
        }

    }

    void sweep() {

        for (int i = 0; i < memory.size(); i++) {
            if (!memory[i].isFree && !memory[i].isReachable) {
                cout << memory[i].name << " collected!" << endl;
                memory[i].isFree = true;
                memory[i].name = "";
                memory[i].references.clear();
            }
        }

        coalesce();

    }

    void garbageCollection() {

        resetMarks();

        for (int i = 0; i < roots.size(); i++) {
            mark(findBlock(roots[i]));                    // CHANGED
        }

        sweep();

        cout << "Garbage Collection completed!" << endl;

    }

};

#endif // MEMORY_MANAGEMENT_ALGORITHM_H
