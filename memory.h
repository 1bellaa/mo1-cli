#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

using namespace std;

struct PageTableEntry {
    bool valid;           // Is page in physical memory?
    int frameNumber;      // Frame number if valid
    bool dirty;           // Has page been modified?
    time_t lastAccess;    // For LRU replacement

    PageTableEntry() : valid(false), frameNumber(-1), dirty(false), lastAccess(0) {}
};

struct Frame {
    bool occupied;
    int processId;
    int pageNumber;

    Frame() : occupied(false), processId(-1), pageNumber(-1) {}
};

class MemoryManager {
private:
    int maxOverallMem;      // Total physical memory
    int memPerFrame;        // Size of each frame/page
    int totalFrames;        // Total number of frames
    int minMemPerProc;      // Min memory per process
    int maxMemPerProc;      // Max memory per process

    vector<Frame> physicalMemory;  // Physical memory frames
    map<string, string> backingStore;  // Backing store (page_key -> data)

    int numPagedIn;         // Statistics
    int numPagedOut;

    int FindVictimFrame();  // Page replacement algorithm (LRU)
    void PageOut(int frameIndex);
    void PageIn(int processId, int pageNumber, int frameIndex);

public:
    MemoryManager();
    void Initialize(int maxOverallMemory, int frameSize, int minMemPerProcess, int maxMemPerProcess);

    bool AllocateMemory(int processId, int memorySize, vector<PageTableEntry>& pageTable);
    void DeallocateMemory(int processId, vector<PageTableEntry>& pageTable);

    bool AccessMemory(int processId, uint32_t address, vector<PageTableEntry>& pageTable, bool isWrite);
    uint16_t ReadMemory(int processId, uint32_t address, vector<PageTableEntry>& pageTable);
    void WriteMemory(int processId, uint32_t address, uint16_t value, vector<PageTableEntry>& pageTable);

    int GetUsedMemory() const;
    int GetFreeMemory() const;
    int GetTotalMemory() const { return maxOverallMem; }
    int GetNumPagedIn() const { return numPagedIn; }
    int GetNumPagedOut() const { return numPagedOut; }
    int GetFrameSize() const { return memPerFrame; }

    void SaveBackingStore();
    void PrintMemoryStatus() const;
};

#endif