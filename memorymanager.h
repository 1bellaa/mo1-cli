#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

/* NTS: pls check correctness na lang ewan q na */

#include <vector>
#include <map>
#include <string>
#include <cstdint>

class Process;

/* Represents a single page in memory with tracking information */
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
    Process* process;     // Owner process, nullptr if free
    int pageNumber;       // Which page is stored here
    bool isFree;

    Frame(int num) : frameNumber(num), process(nullptr), pageNumber(-1), isFree(true) {}
};

/* Main memory manager handling paging, allocation, and backing store operations.
   Manages physical frames, page tables, and coordinates with backing store for swapping. */
class MemoryManager {
private:
    size_t maxOverallMem;
    size_t memPerFrame;
    size_t minMemPerProc;
    size_t maxMemPerProc;

    size_t totalFrames;
    size_t usedMemory;
    std::vector<Frame*> frames;
    std::map<Process*, std::vector<Page>> pageTable;

    uint64_t numPagedIn;
    uint64_t numPagedOut;
    uint64_t accessCounter;

    std::string backingStoreFile;

    /* Finds a victim frame to evict using LRU (Least Recently Used) algorithm */
    int FindVictimFrame();

    /* Writes a page to backing store when evicting from physical memory */
    void PageOut(Process* proc, int pageNum);

    /* Loads a page from backing store into physical memory */
    void PageIn(Process* proc, int pageNum, int frameNum);

public:
    MemoryManager();
    ~MemoryManager();

    /* Initializes memory manager with configuration parameters from config.txt */
    void Initialize(size_t maxMem, size_t frameSize, size_t minProcMem, size_t maxProcMem);

    /* Allocates memory for a process and initializes its page table.
       Returns true if allocation successful, false if not enough memory. */
    bool AllocateMemory(Process* proc, size_t memSize);

    /* Deallocates all memory associated with a process and frees frames */
    void DeallocateMemory(Process* proc);

    /* Handles page fault - brings requested page into physical memory.
       May trigger page replacement if no free frames available. */
    bool HandlePageFault(Process* proc, size_t address);

    /* Validates if a memory address is within the process's allocated space */
    bool IsValidAddress(Process* proc, size_t address);

    /* Reads a uint16 value from memory address. Triggers page fault if needed. */
    uint16_t ReadMemory(Process* proc, size_t address);

    /* Writes a uint16 value to memory address. Triggers page fault if needed. */
    void WriteMemory(Process* proc, size_t address, uint16_t value);

    /* Returns total memory currently in use by processes */
    size_t GetUsedMemory() const { return usedMemory; }

    /* Returns total available memory */
    size_t GetTotalMemory() const { return maxOverallMem; }

    /* Returns free memory available for allocation */
    size_t GetFreeMemory() const { return maxOverallMem - usedMemory; }

    /* Returns total number of page-in operations performed */
    uint64_t GetNumPagedIn() const { return numPagedIn; }

    /* Returns total number of page-out operations performed */
    uint64_t GetNumPagedOut() const { return numPagedOut; }

    /* Gets memory size allocated to a specific process */
    size_t GetProcessMemory(Process* proc);

    /* Returns list of all processes currently using memory */
    std::vector<Process*> GetProcessesInMemory();

    /* Generates process-smi style output showing memory usage */
    void PrintProcessSMI();

    /* Generates vmstat style output with detailed memory statistics */
    void PrintVMStat(uint64_t idleTicks, uint64_t activeTicks, uint64_t totalTicks);
};

#endif