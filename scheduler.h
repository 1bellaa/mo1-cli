#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include <queue>
#include <map>
#include "process.h"
#include "memory.h"

using namespace std;

enum SchedulerType {
    FCFS,
    ROUND_ROBIN
};

struct Config {
    int numCPU;
    SchedulerType type;
    int quantumCycles;
    int batchProcessFreq;
    int minIns;
    int maxIns;
    int delaysPerExec;

    // MO2 memory parameters
    int maxOverallMem;
    int memPerFrame;
    int minMemPerProc;
    int maxMemPerProc;
};

class Scheduler {
private:
    Config config;
    int currentPID;
    int cpuTicks;
    int processCounter;
    bool isRunning;
    bool hasEverGenerated;

    int numCPU;
    SchedulerType type;
    int quantumCycles;
    int batchProcessFreq;
    int minIns;
    int maxIns;
    int delaysPerExec;

    vector<Process*> allProcesses;
    queue<Process*> readyQueue;
    map<int, Process*> coreAssignments;
    map<Process*, int> processQuantumCounters;

    MemoryManager memoryManager;

    int idleCpuTicks;
    int activeCpuTicks;

    void LoadConfig(const string& filename);
    void ScheduleNext(int coreId);

public:
    Scheduler();
    ~Scheduler();

    void Initialize(const string& configFile);
    void Tick();
    void Start();
    void Stop();

    void CreateNewProcess(const string& name, int memorySize = -1);
    Process* GetProcess(const string& name);
    bool TryAssignProcess(Process* proc);

    int GetCoresUsed() const;
    int GetCoresAvailable() const;
    double GetCPUUtilization() const;

    vector<Process*> GetRunningProcesses() const;
    vector<Process*> GetFinishedProcesses() const;

    int GetCPUTicks() const { return cpuTicks; }
    int GetIdleCpuTicks() const { return idleCpuTicks; }
    int GetActiveCpuTicks() const { return activeCpuTicks; }

    MemoryManager* GetMemoryManager() { return &memoryManager; }
};

#endif