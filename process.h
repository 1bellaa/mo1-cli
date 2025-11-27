#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include "instruction.h"
#include "memory.h"

using namespace std;

enum ProcessState {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

class Process {
private:
    string name;
    int pid;
    ProcessState state;
    int currentLine;
    int totalLines;
    int coreAssigned;
    int waitCycles;
    int executionTime;
    int delayCounter;
    time_t finishTime;

    // Memory management
    int memorySize;
    vector<PageTableEntry> pageTable;
    map<string, uint16_t> variables;  // Symbol table (max 32 variables)
    bool hasMemoryError;
    time_t memoryErrorTime;
    uint32_t memoryErrorAddress;

    vector<Instruction*> instructions;
    vector<string> outputLog;

public:
    Process(string processName, int processId, int numInstructions, int delaysPerExec, int memSize = 256);
    ~Process();

    void Execute(int coreId, class MemoryManager* memMgr);
    bool IsFinished() const;
    void PrintInfo() const;
    void AddOutput(const string& output);

    // Memory operations
    uint16_t GetVariable(const string& varName);
    void SetVariable(const string& varName, uint16_t value);
    bool DeclareVariable(const string& varName, uint16_t value);

    uint16_t ReadMemoryAddress(uint32_t address, class MemoryManager* memMgr);
    void WriteMemoryAddress(uint32_t address, uint16_t value, class MemoryManager* memMgr);

    // Getters
    string GetName() const { return name; }
    int GetPID() const { return pid; }
    ProcessState GetState() const { return state; }
    int GetCurrentLine() const { return currentLine; }
    int GetTotalLines() const { return totalLines; }
    int GetCoreAssigned() const { return coreAssigned; }
    int GetWaitCycles() const { return waitCycles; }
    int GetExecutionTime() const { return executionTime; }
    time_t GetFinishTime() const { return finishTime; }
    int GetMemorySize() const { return memorySize; }
    bool HasMemoryError() const { return hasMemoryError; }
    time_t GetMemoryErrorTime() const { return memoryErrorTime; }
    uint32_t GetMemoryErrorAddress() const { return memoryErrorAddress; }

    // Setters
    void SetState(ProcessState s) { state = s; }
    void SetCoreAssigned(int core) { coreAssigned = core; }
    void SetFinishTime(time_t t) { finishTime = t; }
    void IncrementExecutionTime() { executionTime++; }
    void DecrementWait() { if (waitCycles > 0) waitCycles--; }

    vector<PageTableEntry>& GetPageTable() { return pageTable; }
};

#endif