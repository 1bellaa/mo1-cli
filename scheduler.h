#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <string>
#include <vector>
#include <queue>
#include <map>
#include "process.h"

using namespace std;

enum SchedulerType {
    FCFS,
    ROUND_ROBIN
};

struct Config {
    int numCPU = 4;
    SchedulerType type = ROUND_ROBIN;
    int quantumCycles = 5;
    int batchProcessFreq = 1;
    int minIns = 1000;
    int maxIns = 2000;
    int delaysPerExec = 0;
};

class Scheduler {
private:
    SchedulerType type;
    int numCPU;
    int quantumCycles;
    int batchProcessFreq;
    int minIns;
    int maxIns;
    int delaysPerExec;

    queue<Process*> readyQueue;
    vector<Process*> runningProcesses;
    vector<Process*> allProcesses;
    map<int, Process*> coreAssignments;
    map<Process*, int> processQuantumCounters;

    int currentPID;
    int cpuTicks;
    int processCounter;
    bool isRunning;

    Config config;

    void LoadConfig(const string& filename);
    void ScheduleNext(int coreId);

public:
    Scheduler();
    ~Scheduler();

    void Initialize(const string& configFile);
    void Tick();
    void Start();
    void Stop();

    void CreateNewProcess(const string& name);
    Process* GetProcess(const string& name);
    vector<Process*> GetAllProcesses() const { return allProcesses; }

    int GetNumCPU() const { return numCPU; }
    int GetCPUTicks() const { return cpuTicks; }
    bool IsRunning() const { return isRunning; }
    int GetCoresUsed() const;
    int GetCoresAvailable() const;
    double GetCPUUtilization() const;

    vector<Process*> GetRunningProcesses() const;
    vector<Process*> GetFinishedProcesses() const;

    bool TryAssignProcess(Process* proc);
};

#endif