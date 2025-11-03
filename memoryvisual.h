#ifndef MEMORYVISUAL_H
#define MEMORYVISUAL_H

#include <string>
#include "memoryallocator.h"

using namespace std;

class MemoryVisual {
public:
    MemoryVisual(MemoryAllocator* ma);
    string ProcessSmi(int pid);
    string Vmstat();

private:
    MemoryAllocator* ma;
};

#endif