#include "process.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include "memorymanager.h"

using namespace std;

/* Constructor initializes memory manager with no allocations */
MemoryManager::MemoryManager()
    : maxOverallMem(0), memPerFrame(0), minMemPerProc(0), maxMemPerProc(0),
    totalFrames(0), usedMemory(0), numPagedIn(0), numPagedOut(0), accessCounter(0) {
    backingStoreFile = "csopesy-backing-store.txt";
}

/* Destructor frees all frames and clears page tables to prevent memory leaks */
MemoryManager::~MemoryManager() {
    for (auto frame : frames) {
        delete frame;
    }
    frames.clear();
    pageTable.clear();
    processMemorySizes.clear();
}

/* Initializes memory subsystem with configuration parameters.
   Creates physical frame pool for demand paging allocation. */
void MemoryManager::Initialize(size_t maxMem, size_t frameSize, size_t minProcMem, size_t maxProcMem) {
    maxOverallMem = maxMem;
    memPerFrame = frameSize;
    minMemPerProc = minProcMem;
    maxMemPerProc = maxProcMem;

    totalFrames = maxOverallMem / memPerFrame;

    // Create physical frame pool
    for (size_t i = 0; i < totalFrames; i++) {
        frames.push_back(new Frame(i));
    }

    // Initialize backing store file
    ofstream backingStore(backingStoreFile, ios::trunc);
    backingStore << "=== CSOPESY Backing Store ===" << endl;
    backingStore.close();

    cout << "Memory Manager initialized:" << endl;
    cout << "Max memory: " << maxOverallMem << " bytes" << endl;
    cout << "Frame size: " << memPerFrame << " bytes" << endl;
    cout << "Total frames: " << totalFrames << endl;
}

/* Allocates virtual memory for a process using demand paging.
   Creates page table but doesn't load pages until accessed (page fault). */
bool MemoryManager::AllocateMemory(Process* proc, size_t memSize) {
    // Validate: must be power of 2 and within range [64, 65536]
    if (memSize < 64 || memSize > 65536 || (memSize & (memSize - 1)) != 0) {
        return false;
    }

    // Check if enough virtual memory available
    if (usedMemory + memSize > maxOverallMem) {
        return false;
    }

    // Calculate pages needed
    size_t pagesNeeded = (memSize + memPerFrame - 1) / memPerFrame;

    // Create page table entries (not loaded into frames yet)
    vector<Page> pages;
    for (size_t i = 0; i < pagesNeeded; i++) {
        Page page;
        page.pageNumber = i;
        page.frameNumber = -1;
        page.isValid = false;
        page.isDirty = false;
        page.lastAccessTime = 0;
        pages.push_back(page);
    }

    pageTable[proc] = pages;
    processMemorySizes[proc] = memSize;
    proc->SetMemorySize(memSize);
    usedMemory += memSize;

    return true;
}

/* Deallocates process memory and frees all its frames.
   Removes from page table and updates memory accounting. */
void MemoryManager::DeallocateMemory(Process* proc) {
    if (pageTable.find(proc) == pageTable.end()) {
        return;
    }

    vector<Page>& pages = pageTable[proc];

    // Free all frames occupied by this process
    for (auto& page : pages) {
        if (page.isValid && page.frameNumber != -1) {
            frames[page.frameNumber]->isFree = true;
            frames[page.frameNumber]->process = nullptr;
            frames[page.frameNumber]->pageNumber = -1;
        }
    }

    size_t memSize = processMemorySizes[proc];
    usedMemory -= memSize;

    pageTable.erase(proc);
    processMemorySizes.erase(proc);
}

/* Finds victim frame for page replacement using LRU algorithm.
   Returns first free frame, or frame with oldest access time. */
int MemoryManager::FindVictimFrame() {
    // First try to find a free frame
    for (auto frame : frames) {
        if (frame->isFree) {
            return frame->frameNumber;
        }
    }

    // No free frames - use LRU to find victim
    int victimFrame = -1;
    uint64_t oldestAccess = UINT64_MAX;

    for (auto frame : frames) {
        if (frame->process != nullptr && pageTable.find(frame->process) != pageTable.end()) {
            vector<Page>& pages = pageTable[frame->process];
            if (frame->pageNumber < pages.size()) {
                if (pages[frame->pageNumber].lastAccessTime < oldestAccess) {
                    oldestAccess = pages[frame->pageNumber].lastAccessTime;
                    victimFrame = frame->frameNumber;
                }
            }
        }
    }

    return victimFrame;
}

/* Writes evicted page to backing store and logs the operation */
void MemoryManager::PageOut(Process* proc, int pageNum) {
    ofstream backingStore(backingStoreFile, ios::app);

    backingStore << "[PAGE OUT] Process: " << proc->GetName()
        << " | Page: " << pageNum
        << " | Time: " << accessCounter << endl;

    backingStore.close();
    numPagedOut++;
}

/* Loads page from backing store into physical memory frame */
void MemoryManager::PageIn(Process* proc, int pageNum, int frameNum) {
    ofstream backingStore(backingStoreFile, ios::app);

    backingStore << "[PAGE IN] Process: " << proc->GetName()
        << " | Page: " << pageNum
        << " | Frame: " << frameNum
        << " | Time: " << accessCounter << endl;

    backingStore.close();
    numPagedIn++;
}

/* Handles page fault by bringing requested page into memory.
   Implements demand paging with LRU page replacement. */
bool MemoryManager::HandlePageFault(Process* proc, size_t address) {
    if (pageTable.find(proc) == pageTable.end()) {
        return false;
    }

    // Calculate which page is needed
    size_t pageNum = address / memPerFrame;
    vector<Page>& pages = pageTable[proc];

    if (pageNum >= pages.size()) {
        return false;
    }

    // Find frame (free or victim)
    int frameNum = FindVictimFrame();
    if (frameNum == -1) {
        return false;
    }

    // Evict victim page if frame is occupied
    if (!frames[frameNum]->isFree) {
        Process* victimProc = frames[frameNum]->process;
        int victimPage = frames[frameNum]->pageNumber;

        if (victimProc != nullptr && pageTable.find(victimProc) != pageTable.end()) {
            vector<Page>& victimPages = pageTable[victimProc];
            if (victimPage < victimPages.size()) {
                victimPages[victimPage].isValid = false;
                victimPages[victimPage].frameNumber = -1;

                if (victimPages[victimPage].isDirty) {
                    PageOut(victimProc, victimPage);
                }
            }
        }
    }

    // Load requested page
    PageIn(proc, pageNum, frameNum);

    // Update page table
    pages[pageNum].isValid = true;
    pages[pageNum].frameNumber = frameNum;
    pages[pageNum].lastAccessTime = ++accessCounter;
    pages[pageNum].isDirty = false;

    // Update frame
    frames[frameNum]->isFree = false;
    frames[frameNum]->process = proc;
    frames[frameNum]->pageNumber = pageNum;

    return true;
}

/* Validates memory address is within process's allocated range */
bool MemoryManager::IsValidAddress(Process* proc, size_t address) {
    if (processMemorySizes.find(proc) == processMemorySizes.end()) {
        return false;
    }
    return address < processMemorySizes[proc];
}

/* Reads uint16 from memory with automatic page fault handling */
uint16_t MemoryManager::ReadMemory(Process* proc, size_t address) {
    if (!IsValidAddress(proc, address)) {
        throw runtime_error("Memory access violation");
    }

    size_t pageNum = address / memPerFrame;
    vector<Page>& pages = pageTable[proc];

    // Handle page fault if page not in memory
    if (!pages[pageNum].isValid) {
        if (!HandlePageFault(proc, address)) {
            throw runtime_error("Page fault handling failed");
        }
    }

    pages[pageNum].lastAccessTime = ++accessCounter;
    return proc->ReadFromMemory(address);
}

/* Writes uint16 to memory with automatic page fault handling */
void MemoryManager::WriteMemory(Process* proc, size_t address, uint16_t value) {
    if (!IsValidAddress(proc, address)) {
        throw runtime_error("Memory access violation");
    }

    size_t pageNum = address / memPerFrame;
    vector<Page>& pages = pageTable[proc];

    // Handle page fault if page not in memory
    if (!pages[pageNum].isValid) {
        if (!HandlePageFault(proc, address)) {
            throw runtime_error("Page fault handling failed");
        }
    }

    pages[pageNum].lastAccessTime = ++accessCounter;
    pages[pageNum].isDirty = true;
    proc->WriteToMemory(address, value);
}

/* Gets memory size allocated to specific process */
size_t MemoryManager::GetProcessMemory(Process* proc) {
    if (processMemorySizes.find(proc) == processMemorySizes.end()) {
        return 0;
    }
    return processMemorySizes[proc];
}

/* Returns list of all processes with memory allocations */
vector<Process*> MemoryManager::GetProcessesInMemory() {
    vector<Process*> procs;
    for (auto& entry : pageTable) {
        procs.push_back(entry.first);
    }
    return procs;
}

/* Displays nvidia-smi style memory summary */
void MemoryManager::PrintProcessSMI() {
    cout << "\n===================================================" << endl;
    cout << "| PROCESS-SMI V01.00                              |" << endl;
    cout << "===================================================" << endl;

    cout << "CPU-Util: " << fixed << setprecision(1)
        << (usedMemory * 100.0 / maxOverallMem) << "%" << endl;
    cout << "Memory Usage: " << usedMemory << " / " << maxOverallMem << " bytes" << endl;
    cout << "Memory Util: " << fixed << setprecision(1)
        << (usedMemory * 100.0 / maxOverallMem) << "%" << endl;

    cout << "\n===================================================" << endl;
    cout << "Running processes and memory usage:" << endl;
    cout << "---------------------------------------------------" << endl;

    for (auto& entry : processMemorySizes) {
        Process* proc = entry.first;
        size_t memSize = entry.second;

        cout << proc->GetName() << " | " << memSize << " bytes" << endl;
    }

    cout << "===================================================" << endl;
}

/* Displays vmstat style detailed memory statistics */
void MemoryManager::PrintVMStat(uint64_t idleTicks, uint64_t activeTicks, uint64_t totalTicks) {
    cout << "\n===================================================" << endl;
    cout << "VMSTAT - Virtual Memory Statistics" << endl;
    cout << "===================================================" << endl;

    cout << "Total memory: " << maxOverallMem << " bytes" << endl;
    cout << "Used memory: " << usedMemory << " bytes" << endl;
    cout << "Free memory: " << (maxOverallMem - usedMemory) << " bytes" << endl;

    cout << "\nCPU Statistics:" << endl;
    cout << "Idle CPU ticks: " << idleTicks << endl;
    cout << "Active CPU ticks: " << activeTicks << endl;
    cout << "Total CPU ticks: " << totalTicks << endl;

    cout << "\nPaging Statistics:" << endl;
    cout << "Num paged in: " << numPagedIn << endl;
    cout << "Num paged out: " << numPagedOut << endl;

    cout << "===================================================" << endl;
}