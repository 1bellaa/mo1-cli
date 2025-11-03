#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "memoryallocator.h"

using namespace std;

Frame::Frame(int id, size_t frameSize) : frameId(id), ownerPid(-1), vpn(0), lastAccessTick(0) {
    data.resize(frameSize);
}

MemoryAllocator::MemoryAllocator() : maxOverallMem(0), memPerFrame(0), totalFrames(0), tickCounter(0), pagedIn(0), pagedOut(0) {}

MemoryAllocator::~MemoryAllocator() {
    // flush frames to backing store? leaving as-is.
}

void MemoryAllocator::Initialize(size_t maxOverallMem_, size_t memPerFrame_) {
    lock_guard<mutex> g(lock);
    maxOverallMem = maxOverallMem_;
    memPerFrame = memPerFrame_;
    if (memPerFrame == 0) memPerFrame = 256;
    totalFrames = maxOverallMem / memPerFrame;
    frames.clear();
    frames.reserve(totalFrames);
    for (size_t i = 0; i < totalFrames; i++) frames.emplace_back((int)i, memPerFrame);
    pageTable.clear();
    lruList.clear();
    tickCounter = 0;
    pagedIn = 0;
    pagedOut = 0;
}

bool MemoryAllocator::ReserveFramesForProcess(int pid, size_t numFrames) {
    lock_guard<mutex> g(lock);
    (void)numFrames;
    pageTable[pid]; // ensure entry exists
    return true;
}

void MemoryAllocator::FreeFramesByPid(int pid) {
    lock_guard<mutex> g(lock);
    auto it = pageTable.find(pid);
    if (it != pageTable.end()) {
        for (auto& p : it->second) {
            int fid = p.second;
            if (fid >= 0 && fid < (int)frames.size()) {
                frames[fid].ownerPid = -1;
                frames[fid].vpn = 0;
                fill(frames[fid].data.begin(), frames[fid].data.end(), 0);
            }
            lruList.remove(fid);
        }
        pageTable.erase(it);
    }
}

int MemoryAllocator::PickVictimFrame() {
    // find free frame first
    for (auto& fr : frames) {
        if (fr.ownerPid == -1) return fr.frameId;
    }
    // otherwise pick least recently used (back of list)
    if (!lruList.empty()) {
        int victim = lruList.back();
        lruList.pop_back();
        return victim;
    }
    // fallback
    return 0;
}

int MemoryAllocator::LoadPageFromBackingStore(int pid, uint32_t vpn, const std::string& backingStorePath_) {
    lock_guard<mutex> g(lock);
    backingStorePath = backingStorePath_;
    int fid = GetFrameForPidVPN(pid, vpn);
    if (fid != -1) {
        frames[fid].lastAccessTick = tickCounter++;
        // update LRU: move to front
        lruList.remove(fid);
        lruList.push_front(fid);
        return fid;
    }

    int victim = PickVictimFrame();
    // if victim owned -> write to backing store first
    if (frames[victim].ownerPid != -1) {
        EvictFrame(victim, backingStorePath);
        pagedOut++;
    }

    // read from backing store file into frames[victim].data
    ifstream in(backingStorePath, ios::binary);
    if (!in) {
        // no backing store yet: zero page
        fill(frames[victim].data.begin(), frames[victim].data.end(), 0);
    }
    else {
        // Simple text format: lines like: pid vpn <hex bytes...>
        in.clear();
        in.seekg(0, ios::beg);
        string line;
        bool found = false;
        while (getline(in, line)) {
            istringstream iss(line);
            int fpid; unsigned int fvpn;
            if (!(iss >> fpid >> std::hex >> fvpn)) continue;
            if (fpid == pid && fvpn == vpn) {
                // read rest hex bytes
                string hexbyte;
                size_t idx = 0;
                while (iss >> hexbyte && idx < frames[victim].data.size()) {
                    unsigned int b = 0;
                    stringstream ss; ss << std::hex << hexbyte;
                    ss >> b;
                    frames[victim].data[idx++] = static_cast<uint8_t>(b & 0xFF);
                }
                for (; idx < frames[victim].data.size(); ++idx) frames[victim].data[idx] = 0;
                found = true;
                break;
            }
        }
        if (!found) {
            fill(frames[victim].data.begin(), frames[victim].data.end(), 0);
        }
    }

    // update frame metadata
    frames[victim].ownerPid = pid;
    frames[victim].vpn = vpn;
    frames[victim].lastAccessTick = tickCounter++;
    pageTable[pid][vpn] = victim;
    lruList.remove(victim);
    lruList.push_front(victim);
    pagedIn++;
    return victim;
}

int MemoryAllocator::EnsurePageLoaded(int pid, uint32_t vpn, const std::string& backingStorePath_) {
    // wrapper that tries to load and returns frame id
    int fid = GetFrameForPidVPN(pid, vpn);
    if (fid != -1) return fid;
    return LoadPageFromBackingStore(pid, vpn, backingStorePath_);
}

void MemoryAllocator::EvictFrame(int frameId, const string& backingStorePath_) {
    lock_guard<mutex> g(lock);
    if (frameId < 0 || frameId >= (int)frames.size()) return;
    Frame& fr = frames[frameId];
    if (fr.ownerPid == -1) return;

    // append line: pid vpn <hex bytes>
    ofstream out(backingStorePath_, std::ios::app);
    if (out) {
        out << fr.ownerPid << " " << std::hex << fr.vpn;
        out << std::dec;
        for (auto b : fr.data) {
            out << " " << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        }
        out << "\n";
    }

    // remove mapping from pageTable
    auto pit = pageTable.find(fr.ownerPid);
    if (pit != pageTable.end()) {
        pit->second.erase(fr.vpn);
        if (pit->second.empty()) pageTable.erase(pit);
    }

    fr.ownerPid = -1;
    fr.vpn = 0;
    fill(fr.data.begin(), fr.data.end(), 0);
    lruList.remove(frameId);
}

bool MemoryAllocator::ReadPhysical(int pid, uint32_t physAddr, uint16_t& outValue) {
    lock_guard<mutex> g(lock);
    size_t frameSize = memPerFrame;
    uint32_t vpn = physAddr / frameSize;
    uint32_t offset = physAddr % frameSize;
    int fid = GetFrameForPidVPN(pid, vpn);
    if (fid == -1) return false;
    if (offset + sizeof(uint16_t) > frames[fid].data.size()) return false;
    outValue = frames[fid].data[offset] | (frames[fid].data[offset + 1] << 8);
    frames[fid].lastAccessTick = tickCounter++;
    lruList.remove(fid); lruList.push_front(fid);
    return true;
}

bool MemoryAllocator::WritePhysical(int pid, uint32_t physAddr, uint16_t value) {
    lock_guard<mutex> g(lock);
    size_t frameSize = memPerFrame;
    uint32_t vpn = physAddr / frameSize;
    uint32_t offset = physAddr % frameSize;
    int fid = GetFrameForPidVPN(pid, vpn);
    if (fid == -1) return false;
    if (offset + sizeof(uint16_t) > frames[fid].data.size()) return false;
    frames[fid].data[offset] = value & 0xFF;
    frames[fid].data[offset + 1] = (value >> 8) & 0xFF;
    frames[fid].lastAccessTick = tickCounter++;
    lruList.remove(fid); lruList.push_front(fid);
    return true;
}

int MemoryAllocator::GetFrameForPidVPN(int pid, uint32_t vpn) {
    auto pit = pageTable.find(pid);
    if (pit == pageTable.end()) return -1;
    auto q = pit->second.find(vpn);
    if (q == pit->second.end()) return -1;
    return q->second;
}

size_t MemoryAllocator::GetTotalMemory() const { return maxOverallMem; }
size_t MemoryAllocator::GetUsedMemory() const {
    size_t used = 0;
    for (auto& f : frames) if (f.ownerPid != -1) used += memPerFrame;
    return used;
}
size_t MemoryAllocator::GetFreeMemory() const { return GetTotalMemory() - GetUsedMemory(); }
size_t MemoryAllocator::GetFrameSize() const { return memPerFrame; }
size_t MemoryAllocator::GetTotalFrames() const { return totalFrames; }

void MemoryAllocator::Tick() {
    lock_guard<mutex> g(lock);
    tickCounter++;
}