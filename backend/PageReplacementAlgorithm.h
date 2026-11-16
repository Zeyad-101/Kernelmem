#ifndef PAGE_REPLACEMENT_ALGORITHM_H
#define PAGE_REPLACEMENT_ALGORITHM_H

#include <iostream>
#include <vector>
#include <string>
#include <memory>

class pagereplacement {
public:
    std::string name;
    int fault{0};
    int hit{0};
    int size;
    std::vector<int> frames;

    pagereplacement(int framesCount) : size(framesCount) {}
    virtual ~pagereplacement() = default;

    virtual void access(int page) = 0;
    virtual std::vector<int> getRefBits() const { return {}; }
    virtual int getLastReplacedIndex() const { return -1; }
};

class fifo : public pagereplacement {
public:
    int ptr{0};
    int lastReplaced{-1};

    fifo(int framesCount) : pagereplacement(framesCount) {
        name = "FIFO";
        frames.resize(size, -1);
    }

    void access(int page) override {
        bool found = false;
        for (int i = 0; i < size; i++) {
            if (frames[i] == page) {
                found = true;
                break;
            }
        }
        if (found) {
            hit++;
            lastReplaced = -1;
        } else {
            fault++;
            lastReplaced = ptr;
            frames[ptr] = page;
            ptr = (ptr + 1) % size;
        }
    }

    int getLastReplacedIndex() const override { return lastReplaced; }
};

class LRU : public pagereplacement {
public:
    std::vector<int> lastUsed;
    int timeStep{0};
    int lastReplaced{-1};

    LRU(int framesCount) : pagereplacement(framesCount) {
        name = "LRU";
        for (int i = 0; i < size; i++) {
            frames.push_back(-1);
            lastUsed.push_back(-1);
        }
    }

    void access(int page) override {
        timeStep++;
        for (int i = 0; i < size; i++) {
            if (frames[i] == page) {
                hit++;
                lastUsed[i] = timeStep;
                lastReplaced = -1;
                return;
            }
        }
        fault++;
        int replaceId = 0;
        for (int i = 0; i < size; i++) {
            if (frames[i] == -1) {
                replaceId = i;
                break;
            }
            if (lastUsed[i] < lastUsed[replaceId]) {
                replaceId = i;
            }
        }
        frames[replaceId] = page;
        lastUsed[replaceId] = timeStep;
        lastReplaced = replaceId;
    }

    int getLastReplacedIndex() const override { return lastReplaced; }
};

class Clock : public pagereplacement {
public:
    std::vector<int> refBit;
    int index{0};
    int lastReplaced{-1};

    Clock(int framesCount) : pagereplacement(framesCount) {
        name = "Clock";
        for (int i = 0; i < size; i++) {
            frames.push_back(-1);
            refBit.push_back(0);
        }
    }

    void access(int page) override {
        for (int i = 0; i < size; i++) {
            if (frames[i] == page) {
                hit++;
                refBit[i] = 1;
                lastReplaced = -1;
                return;
            }
        }

        fault++;
        for (;;) {
            if (frames[index] == -1 || refBit[index] == 0) {
                frames[index] = page;
                refBit[index] = 1;
                lastReplaced = index;
                index = (index + 1) % size;
                break;
            } else {
                refBit[index] = 0;
                index = (index + 1) % size;
            }
        }
    }

    std::vector<int> getRefBits() const override { return refBit; }
    int getLastReplacedIndex() const override { return lastReplaced; }
};

struct PageStepRecord {
    int page;
    std::vector<int> frames;
    std::vector<int> refBits;
    bool isHit;
    int replacedFrame;
};

class PageReplacementSim {
private:
    std::unique_ptr<pagereplacement> algoInstance;
    std::string currentAlgoName;
    int frameCount{3};
    std::vector<PageStepRecord> history;

public:
    void init(int frames, const std::string& algoName) {
        frameCount = frames;
        currentAlgoName = algoName;
        history.clear();

        if (algoName == "LRU") {
            algoInstance = std::make_unique<LRU>(frames);
        } else if (algoName == "Clock") {
            algoInstance = std::make_unique<Clock>(frames);
        } else {
            algoInstance = std::make_unique<fifo>(frames);
        }
    }

    void step(int page) {
        if (!algoInstance) return;

        int prevFaults = algoInstance->fault;
        algoInstance->access(page);
        bool isHit = (algoInstance->fault == prevFaults);

        PageStepRecord rec;
        rec.page = page;
        rec.frames = algoInstance->frames;
        rec.refBits = algoInstance->getRefBits();
        rec.isHit = isHit;
        rec.replacedFrame = isHit ? -1 : algoInstance->getLastReplacedIndex();

        history.push_back(rec);
    }

    void runAll(const std::vector<int>& pages) {
        for (int p : pages) step(p);
    }

    int getFaultCount() const { return algoInstance ? algoInstance->fault : 0; }
    int getHitCount() const { return algoInstance ? algoInstance->hit : 0; }
    int getFrameCount() const { return frameCount; }
    const std::string& getAlgoName() const { return currentAlgoName; }
    const std::vector<PageStepRecord>& getHistory() const { return history; }
};

#endif // PAGE_REPLACEMENT_ALGORITHM_H
