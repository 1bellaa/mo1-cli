#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <vector>
#include <map>
#include <string>
#include <cstdint>

using namespace std;

class Process;

/* Represents a single page in virtual memory with state tracking */
struct Page {
    int pageNumber;
    int frameNumber;      // -1 if not in physical memory
    bool isValid;         // True if page is in physical memory
    bool isDirty;         // True if page has been modified
    uint64_t lastAccessTime;

    Page() : pageNumber(-1), frameNumber(-1), isValid(false), isDirty(false), lastAccessTime(0) {}
};

/* Represents a physical memory frame */
struct Frame {
    int frameNumber;
    Process* process;    
    int pageNumber;      
    bool isFree;

    Frame(int num) : frameNumber(num), process(nullptr), pageNumber(-1), isFree(true) {}
};

/* Main memory manager implementing demand paging with LRU replacement.
   Manages physical frames, page tables, and backing store operations.
   Memory is efficiently allocated only when needed (demand paging). */
class MemoryManager {
private:
    size_t maxOverallMem;
    size_t memPerFrame;
    size_t minMemPerProc;
    size_t maxMemPerProc;

    size_t totalFrames;
    size_t usedMemory;
    vector<Frame*> frames;
    map<Process*, vector<Page>> pageTable;
    map<Process*, size_t> processMemorySizes;

    uint64_t numPagedIn;
    uint64_t numPagedOut;
    uint64_t accessCounter;

    string backingStoreFile;

    /* Finds a victim frame to evict using LRU algorithm.
       Returns frame number, or -1 if no suitable frame found. */
    int FindVictimFrame();

    /* Writes a page to backing store during eviction.
       Logs the operation to csopesy-backing-store.txt. */
    void PageOut(Process* proc, int pageNum);

    /* Loads a page from backing store into physical memory.
       Updates page table and frame assignments. */
    void PageIn(Process* proc, int pageNum, int frameNum);

public:
    MemoryManager();
    ~MemoryManager();

    void Initialize(size_t maxMem, size_t frameSize, size_t minProcMem, size_t maxProcMem);

    bool AllocateMemory(Process* proc, size_t memSize);
    void DeallocateMemory(Process* proc);

    bool HandlePageFault(Process* proc, size_t address);
    bool IsValidAddress(Process* proc, size_t address);

    uint16_t ReadMemory(Process* proc, size_t address);
    void WriteMemory(Process* proc, size_t address, uint16_t value);

    size_t GetUsedMemory() const { return usedMemory; }
    size_t GetTotalMemory() const { return maxOverallMem; }
    size_t GetFreeMemory() const { return maxOverallMem - usedMemory; }
    uint64_t GetNumPagedIn() const { return numPagedIn; }
    uint64_t GetNumPagedOut() const { return numPagedOut; }
    size_t GetProcessMemory(Process* proc);
    size_t GetMinMemPerProc() const { return minMemPerProc; }
    size_t GetMaxMemPerProc() const { return maxMemPerProc; }

    vector<Process*> GetProcessesInMemory();
    void PrintProcessSMI();
    void PrintVMStat(uint64_t idleTicks, uint64_t activeTicks, uint64_t totalTicks);
};

#endif