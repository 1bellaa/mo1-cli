#ifndef MEMORYALLOCATOR_H
#define MEMORYALLOCATOR_H

#include <vector>
#include <string>
#include <mutex>
#include <list>
#include <map>
#include <cstdint>

using namespace std;

struct Frame {
    int frameId;
    int ownerPid;
    uint32_t vpn;
    uint64_t lastAccessTick;
    vector<uint8_t> data;

    Frame(int id, size_t frameSize);
};

class MemoryAllocator {
private:
    size_t maxOverallMem;
    size_t memPerFrame;
    size_t totalFrames;

    vector<Frame> frames;
    map<int, map<uint32_t, int>> pageTable;  // pid -> (vpn -> frameId)
    list<int> lruList;  // Frame IDs in LRU order (front = most recent)

    uint64_t tickCounter;
    uint64_t pagedIn;
    uint64_t pagedOut;

    string backingStorePath;
    mutex lock;

    int PickVictimFrame();
    void EvictFrame(int frameId, const string& backingStorePath_);
    int GetFrameForPidVPN(int pid, uint32_t vpn);

public:
    MemoryAllocator();
    ~MemoryAllocator();

    void Initialize(size_t maxOverallMem_, size_t memPerFrame_);
    bool ReserveFramesForProcess(int pid, size_t numFrames);
    void FreeFramesByPid(int pid);
    int LoadPageFromBackingStore(int pid, uint32_t vpn, const string& backingStorePath_);
    int EnsurePageLoaded(int pid, uint32_t vpn, const string& backingStorePath_);
    bool ReadPhysical(int pid, uint32_t physAddr, uint16_t& outValue);
    bool WritePhysical(int pid, uint32_t physAddr, uint16_t value);

    size_t GetTotalMemory() const;
    size_t GetUsedMemory() const;
    size_t GetFreeMemory() const;
    size_t GetFrameSize() const;
    size_t GetTotalFrames() const;
    uint64_t GetPagedInCount() const { return pagedIn; }
    uint64_t GetPagedOutCount() const { return pagedOut; }

    void Tick();
};

#endif 