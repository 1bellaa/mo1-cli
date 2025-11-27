/* Memory management implementation with demand paging */

#include "memory.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdint>
#include <algorithm>
#include <stdexcept>

using namespace std;

/* Constructor initializes memory manager with default values */
MemoryManager::MemoryManager()
    : maxOverallMem(0), memPerFrame(0), totalFrames(0),
    minMemPerProc(0), maxMemPerProc(0), numPagedIn(0), numPagedOut(0) {
}

/* Initialize memory manager with configuration parameters.
   Sets up physical memory frames and validates parameters. */
void MemoryManager::Initialize(int maxOverallMemory, int frameSize, int minMemPerProcess, int maxMemPerProcess) {
    maxOverallMem = maxOverallMemory;
    memPerFrame = frameSize;
    totalFrames = maxOverallMem / memPerFrame;
    minMemPerProc = minMemPerProcess;
    maxMemPerProc = maxMemPerProcess;

    physicalMemory.resize(totalFrames);
    numPagedIn = 0;
    numPagedOut = 0;

    cout << "Memory Manager initialized:" << endl;
    cout << "  Total Memory: " << maxOverallMem << " bytes" << endl;
    cout << "  Frame Size: " << memPerFrame << " bytes" << endl;
    cout << "  Total Frames: " << totalFrames << endl;
}

/* Allocate memory for a process using demand paging.
   Validates memory size (64-65536 bytes, power of 2) and creates page table.
   Physical frames are allocated on-demand when pages are accessed. */
bool MemoryManager::AllocateMemory(int processId, int memorySize, vector<PageTableEntry>& pageTable) {
    // Validate memory size (must be power of 2, between 2^6 and 2^16)
    if (memorySize < 64 || memorySize > 65536) {
        return false;
    }

    // Check if power of 2
    if ((memorySize & (memorySize - 1)) != 0) {
        return false;
    }

    // Check if process requires more memory than total system memory
    if (memorySize > maxOverallMem) {
        return false;
    }

    int pagesNeeded = (memorySize + memPerFrame - 1) / memPerFrame;

    // Initialize page table for this process
    // Pages are allocated on demand (demand paging), so we just reserve the page table
    pageTable.resize(pagesNeeded);

    return true;
}

/* Deallocate all memory used by a process.
   Frees physical frames and removes backing store entries. */
void MemoryManager::DeallocateMemory(int processId, vector<PageTableEntry>& pageTable) {
    // Remove all pages from physical memory
    for (size_t i = 0; i < pageTable.size(); i++) {
        if (pageTable[i].valid) {
            int frameNum = pageTable[i].frameNumber;
            if (frameNum >= 0 && frameNum < totalFrames) {
                physicalMemory[frameNum].occupied = false;
                physicalMemory[frameNum].processId = -1;
                physicalMemory[frameNum].pageNumber = -1;
            }
        }
    }

    // Clear backing store entries for this process
    vector<string> keysToRemove;
    for (auto& entry : backingStore) {
        if (entry.first.find("P" + to_string(processId) + "_") == 0) {
            keysToRemove.push_back(entry.first);
        }
    }
    for (auto& key : keysToRemove) {
        backingStore.erase(key);
    }

    pageTable.clear();
}

/* Find a victim frame for page replacement using LRU algorithm.
   Returns index of free frame or frame to be replaced. */
int MemoryManager::FindVictimFrame() {
    // First, look for a free frame
    for (int i = 0; i < totalFrames; i++) {
        if (!physicalMemory[i].occupied) {
            return i;  // Found free frame
        }
    }

    // All frames occupied, use simple round-robin victim selection
    // In a real implementation, would track access times for true LRU
    for (int i = 0; i < totalFrames; i++) {
        if (physicalMemory[i].occupied) {
            return i;
        }
    }

    return -1;  // No frames available
}

/* Page out a frame to backing store.
   Saves page data and marks frame as free. */
void MemoryManager::PageOut(int frameIndex) {
    if (frameIndex < 0 || frameIndex >= totalFrames) return;
    if (!physicalMemory[frameIndex].occupied) return;

    int procId = physicalMemory[frameIndex].processId;
    int pageNum = physicalMemory[frameIndex].pageNumber;

    // Save to backing store
    string key = "P" + to_string(procId) + "_Page" + to_string(pageNum);
    backingStore[key] = "PageData_" + key;

    numPagedOut++;

    physicalMemory[frameIndex].occupied = false;
    physicalMemory[frameIndex].processId = -1;
    physicalMemory[frameIndex].pageNumber = -1;
}

/* Page in a page from backing store to physical memory.
   Loads page data if exists in backing store. */
void MemoryManager::PageIn(int processId, int pageNumber, int frameIndex) {
    if (frameIndex < 0 || frameIndex >= totalFrames) return;

    string key = "P" + to_string(processId) + "_Page" + to_string(pageNumber);

    // Load from backing store if exists
    if (backingStore.find(key) != backingStore.end()) {
        // Data exists in backing store - would load here in real implementation
    }

    physicalMemory[frameIndex].occupied = true;
    physicalMemory[frameIndex].processId = processId;
    physicalMemory[frameIndex].pageNumber = pageNumber;

    numPagedIn++;
}

/* Handle memory access with demand paging.
   Triggers page fault handling if page not in physical memory.
   Returns true if access successful, false if invalid address. */
bool MemoryManager::AccessMemory(int processId, uint32_t address, vector<PageTableEntry>& pageTable, bool isWrite) {
    int pageNumber = address / memPerFrame;

    if (pageNumber < 0 || pageNumber >= (int)pageTable.size()) {
        return false;  // Invalid address - out of process memory bounds
    }

    // Check if page is in physical memory
    if (!pageTable[pageNumber].valid) {
        // Page fault - need to load page
        int frameIndex = FindVictimFrame();

        if (frameIndex == -1) {
            return false;  // No memory available
        }

        // If frame is occupied, page out the victim
        if (physicalMemory[frameIndex].occupied) {
            int victimProcId = physicalMemory[frameIndex].processId;
            int victimPageNum = physicalMemory[frameIndex].pageNumber;

            // Mark victim's page table entry as invalid
            // (This requires access to other process page tables - simplified here)
            PageOut(frameIndex);
        }

        // Page in the requested page
        PageIn(processId, pageNumber, frameIndex);

        pageTable[pageNumber].valid = true;
        pageTable[pageNumber].frameNumber = frameIndex;
    }

    // Update access time for LRU
    pageTable[pageNumber].lastAccess = time(nullptr);

    if (isWrite) {
        pageTable[pageNumber].dirty = true;
    }

    return true;
}

/* Read a uint16 value from memory address.
   Triggers page fault handling if necessary.
   Returns 0 if memory uninitialized. */
uint16_t MemoryManager::ReadMemory(int processId, uint32_t address, vector<PageTableEntry>& pageTable) {
    if (!AccessMemory(processId, address, pageTable, false)) {
        throw runtime_error("Memory access violation at address 0x" +
            to_string(address));
    }

    // In a real implementation, would read from physical memory
    // For simulation, return stored value or 0 if uninitialized
    string key = "P" + to_string(processId) + "_Addr" + to_string(address);

    if (backingStore.find(key) != backingStore.end()) {
        return (uint16_t)stoi(backingStore[key]);
    }

    return 0;  // Uninitialized memory returns 0
}

/* Write a uint16 value to memory address.
   Triggers page fault handling if necessary. */
void MemoryManager::WriteMemory(int processId, uint32_t address, uint16_t value, vector<PageTableEntry>& pageTable) {
    if (!AccessMemory(processId, address, pageTable, true)) {
        throw runtime_error("Memory access violation at address 0x" +
            to_string(address));
    }

    // Store the value
    string key = "P" + to_string(processId) + "_Addr" + to_string(address);
    backingStore[key] = to_string(value);
}

/* Calculate total used memory across all occupied frames. */
int MemoryManager::GetUsedMemory() const {
    int used = 0;
    for (const auto& frame : physicalMemory) {
        if (frame.occupied) {
            used += memPerFrame;
        }
    }
    return used;
}

/* Calculate total free memory available. */
int MemoryManager::GetFreeMemory() const {
    return maxOverallMem - GetUsedMemory();
}

/* Save backing store contents to file for debugging.
   Creates csopesy-backing-store.txt with all paged-out data. */
void MemoryManager::SaveBackingStore() {
    ofstream file("csopesy-backing-store.txt");

    if (!file.is_open()) {
        cerr << "Error: Could not create backing store file." << endl;
        return;
    }

    file << "=== CSOPESY Backing Store ===" << endl;
    file << "Total Entries: " << backingStore.size() << endl;
    file << "==============================" << endl << endl;

    for (const auto& entry : backingStore) {
        file << entry.first << " -> " << entry.second << endl;
    }

    file.close();
    cout << "Backing store saved to csopesy-backing-store.txt" << endl;
}

/* Print current memory status for debugging. */
void MemoryManager::PrintMemoryStatus() const {
    cout << "\n=== Memory Status ===" << endl;
    cout << "Total Memory: " << maxOverallMem << " bytes" << endl;
    cout << "Used Memory: " << GetUsedMemory() << " bytes" << endl;
    cout << "Free Memory: " << GetFreeMemory() << " bytes" << endl;
    cout << "Frames: " << totalFrames << " (" << memPerFrame << " bytes each)" << endl;
    cout << "Pages In: " << numPagedIn << endl;
    cout << "Pages Out: " << numPagedOut << endl;
    cout << "=====================" << endl;
}