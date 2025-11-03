#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "instruction.h"

using namespace std;

enum ProcessState {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

//class Instruction; 

class Process {
private:
    string name;
    int pid;
    ProcessState state;
    int currentLine;
    int totalLines;
    vector<Instruction*> instructions;
    map<string, uint16_t> variables;
    vector<string> outputLog;
    int coreAssigned;
    int waitCycles;
    int executionTime;
    int delayCounter;
    time_t finishTime;

    size_t memorySize;
    map<size_t, uint16_t> memoryMap;  // Virtual memory storage (address -> value)
    bool hasMemoryViolation;
    time_t violationTime;
    size_t violationAddress;

public:
    //Process(string processName, int processId, int numInstructions, int delaysPerExec);
    Process(string processName, int processId, int numInstructions, int delaysPerExec, size_t memSize = 0);
    ~Process();

    void Execute(int coreId);
    bool IsFinished() const;
    void PrintInfo() const;

    string GetName() const { return name; }
    int GetPID() const { return pid; }
    ProcessState GetState() const { return state; }
    void SetState(ProcessState newState) { state = newState; }
    int GetCurrentLine() const { return currentLine; }
    int GetTotalLines() const { return totalLines; }
    int GetCoreAssigned() const { return coreAssigned; }
    void SetCoreAssigned(int core) { coreAssigned = core; }
    int GetWaitCycles() const { return waitCycles; }
    void DecrementWait() { if (waitCycles > 0) waitCycles--; }
    int GetExecutionTime() const { return executionTime; }
    void IncrementExecutionTime() { executionTime++; }

    void AddOutput(const string& output);
    vector<string> GetOutputLog() const { return outputLog; }

    uint16_t GetVariable(const string& varName);
    void SetVariable(const string& varName, uint16_t value);
    int GetVariableCount() const { return variables.size(); }

    void SetFinishTime(time_t t) { finishTime = t; }
    time_t GetFinishTime() const { return finishTime; }

    size_t GetMemorySize() const { return memorySize; }
    void SetMemorySize(size_t size) { memorySize = size; }
	uint16_t ReadFromMemory(size_t address); // implement later
	void WriteToMemory(size_t address, uint16_t value); // implement later

    bool HasMemoryViolation() const { return hasMemoryViolation; } 
    void SetMemoryViolation(size_t address); // implement later ndjkfns
    time_t GetViolationTime() const { return violationTime; }
    size_t GetViolationAddress() const { return violationAddress; }

    void AddInstruction(Instruction* instr);
    void ClearInstructions();
};

#endif