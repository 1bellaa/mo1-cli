#include <sstream>
#include <iomanip>
#include "memoryvisual.h"

using namespace std;

MemoryVisual::MemoryVisual(MemoryAllocator* ma_) : ma(ma_) {}

string MemoryVisual::ProcessSmi(int pid) {
    ostringstream ss;
    if (pid >= 0) {
        ss << "Process-SMI (pid=" << pid << ")\n";
    }
    else {
        ss << "System Process-SMI\n";
    }
    ss << "Total Memory: " << ma->GetTotalMemory() << " bytes\n";
    ss << "Used Memory: " << ma->GetUsedMemory() << " bytes\n";
    ss << "Free Memory: " << ma->GetFreeMemory() << " bytes\n";
    ss << "Frame Size: " << ma->GetFrameSize() << " bytes\n";
    ss << "Total Frames: " << ma->GetTotalFrames() << "\n";
    ss << "Pages paged in: " << ma->GetPagedInCount() << "\n";
    ss << "Pages paged out: " << ma->GetPagedOutCount() << "\n";
    return ss.str();
}

string MemoryVisual::Vmstat() {
    ostringstream ss;
    ss << "VMSTAT\n";
    ss << "Total Memory: " << ma->GetTotalMemory() << " bytes\n";
    ss << "Used Memory: " << ma->GetUsedMemory() << " bytes\n";
    ss << "Free Memory: " << ma->GetFreeMemory() << " bytes\n";
    ss << "Pages Paged In: " << ma->GetPagedInCount() << "\n";
    ss << "Pages Paged Out: " << ma->GetPagedOutCount() << "\n";
    ss << "Note: CPU ticks and per-core stats are reported by Scheduler.\n";
    return ss.str();
}