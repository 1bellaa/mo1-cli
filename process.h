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

public:
    Process(string processName, int processId, int numInstructions, int delaysPerExec);
    ~Process();

    void Execute();
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
};

#endif