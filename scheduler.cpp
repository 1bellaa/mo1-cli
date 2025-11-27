/* Process scheduling logic and scheduling algorithm with memory management */

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "scheduler.h"

using namespace std;

/* Constructor initializes the scheduler with default configuration values
   if config.txt is not found. Includes memory management parameters and
   idle/active CPU tick tracking for vmstat functionality. */
Scheduler::Scheduler() : currentPID(1), cpuTicks(0),
processCounter(1), isRunning(false), hasEverGenerated(false),
idleCpuTicks(0), activeCpuTicks(0) {
    config.numCPU = 4;
    config.type = ROUND_ROBIN;
    config.quantumCycles = 5;
    config.batchProcessFreq = 1;
    config.minIns = 1000;
    config.maxIns = 2000;
    config.delaysPerExec = 0;
    config.maxOverallMem = 16384;
    config.memPerFrame = 256;
    config.minMemPerProc = 256;
    config.maxMemPerProc = 1024;

    numCPU = config.numCPU;
    type = config.type;
    quantumCycles = config.quantumCycles;
    batchProcessFreq = config.batchProcessFreq;
    minIns = config.minIns;
    maxIns = config.maxIns;
    delaysPerExec = config.delaysPerExec;
}

/* Destructor cleans up all dynamically allocated processes and deallocates their memory.
   Called when the console terminates, ensuring no memory leaks.
   Now includes memory deallocation through memory manager. */
Scheduler::~Scheduler() {
    for (auto proc : allProcesses) {
        memoryManager.DeallocateMemory(proc->GetPID(), proc->GetPageTable());
        delete proc;
    }
}

/* Loads configuration from file and initializes CPU cores and memory manager.
   This is called by Console::Initialize() to set up the scheduling environment.
   Now includes memory manager initialization with configured parameters. */
void Scheduler::Initialize(const string& configFile) {
    LoadConfig(configFile);

    numCPU = config.numCPU;
    type = config.type;
    quantumCycles = config.quantumCycles;
    batchProcessFreq = config.batchProcessFreq;
    minIns = config.minIns;
    maxIns = config.maxIns;
    delaysPerExec = config.delaysPerExec;

    for (int i = 0; i < numCPU; i++) {
        coreAssignments[i] = nullptr;
    }

    // Initialize memory manager with configured parameters
    memoryManager.Initialize(config.maxOverallMem, config.memPerFrame,
        config.minMemPerProc, config.maxMemPerProc);
}

/* Reads scheduler configuration from config.txt, setting algorithm type and process parameters.
   Falls back to default values if file cannot be opened.
   Now includes memory management parameters (max-overall-mem, mem-per-frame,
   min-mem-per-proc, max-mem-per-proc). */
void Scheduler::LoadConfig(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Warning: Could not open config file, using defaults" << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string key, value;

        if (iss >> key >> value) {
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            if (key == "num-cpu") config.numCPU = stoi(value);
            else if (key == "scheduler") {
                if (value == "fcfs") config.type = FCFS;
                else if (value == "rr") config.type = ROUND_ROBIN;
            }
            else if (key == "quantum-cycles") config.quantumCycles = stoi(value);
            else if (key == "batch-process-freq") config.batchProcessFreq = stoi(value);
            else if (key == "min-ins") config.minIns = stoi(value);
            else if (key == "max-ins") config.maxIns = stoi(value);
            else if (key == "delays-per-exec") config.delaysPerExec = stoi(value);
            else if (key == "max-overall-mem") config.maxOverallMem = stoi(value);
            else if (key == "mem-per-frame") config.memPerFrame = stoi(value);
            else if (key == "min-mem-per-proc") config.minMemPerProc = stoi(value);
            else if (key == "max-mem-per-proc") config.maxMemPerProc = stoi(value);
        }
    }

    file.close();
}

/* Executes one CPU cycle across all cores.
   Generates new processes based on frequency, executes running processes,
   handles preemption for Round Robin, and manages process state transitions.
   Now tracks idle/active CPU ticks for vmstat, deallocates memory when processes finish,
   and rounds process memory to nearest power of 2 during automatic generation. */
void Scheduler::Tick() {
    cpuTicks++;

    // Track idle/active CPU ticks for vmstat
    int coresInUse = 0;
    for (int i = 0; i < numCPU; i++) {
        if (coreAssignments[i] != nullptr && !coreAssignments[i]->IsFinished()) {
            coresInUse++;
        }
    }

    if (coresInUse > 0) {
        activeCpuTicks += coresInUse;
    }
    idleCpuTicks += (numCPU - coresInUse);

    // Generate new processes if scheduler is running
    if (isRunning && cpuTicks % batchProcessFreq == 0) {
        string processName = "process" + to_string(processCounter++);
        int memSize = config.minMemPerProc + (rand() % (config.maxMemPerProc - config.minMemPerProc + 1));
        // Round to nearest power of 2
        if (memSize > 0) {
            int power = (int)ceil(log2(memSize));
            memSize = 1 << power;
            // Clamp to valid range
            if (memSize < 64) memSize = 64;
            if (memSize > 65536) memSize = 65536;
        }
        CreateNewProcess(processName, memSize);
    }

    /* FCFS scheduling algorithm */
    if (type == FCFS) {
        // Execute processes on cores
        for (int i = 0; i < numCPU; ++i) {
            Process* proc = coreAssignments[i];
            if (proc != nullptr) {
                if (proc->IsFinished()) {
                    if (proc->GetFinishTime() == 0)
                        proc->SetFinishTime(time(nullptr));
                    proc->SetCoreAssigned(-1);
                    memoryManager.DeallocateMemory(proc->GetPID(), proc->GetPageTable());
                    coreAssignments[i] = nullptr;
                    continue;
                }

                proc->Execute(i, &memoryManager);
                proc->IncrementExecutionTime();

                if (proc->GetState() == WAITING) {
                    proc->SetCoreAssigned(-1);
                    coreAssignments[i] = nullptr;
                    processQuantumCounters.erase(proc);
                    continue;
                }

                if (proc->IsFinished()) {
                    if (proc->GetFinishTime() == 0)
                        proc->SetFinishTime(time(nullptr));
                    proc->SetCoreAssigned(-1);
                    memoryManager.DeallocateMemory(proc->GetPID(), proc->GetPageTable());
                    coreAssignments[i] = nullptr;
                }
            }
        }

        // Assign processes from ready queue to idle cores
        for (int i = 0; i < numCPU; ++i) {
            while (!readyQueue.empty() && readyQueue.front()->IsFinished()) {
                readyQueue.pop();
            }

            if (coreAssignments[i] == nullptr && !readyQueue.empty()) {
                Process* nextProc = readyQueue.front();
                readyQueue.pop();

                if (nextProc->IsFinished()) {
                    i--;
                    continue;
                }

                nextProc->SetState(RUNNING);
                nextProc->SetCoreAssigned(i);
                coreAssignments[i] = nextProc;
            }
        }

        // Handle waiting processes
        for (auto proc : allProcesses) {
            if (proc->GetState() == WAITING) {
                proc->DecrementWait();
                if (proc->GetWaitCycles() == 0) {
                    proc->SetState(READY);
                    readyQueue.push(proc);
                }
            }
        }

        return;
    }

    /* Round Robin scheduling algorithm */
    // Execute processes on cores
    for (int i = 0; i < numCPU; ++i) {
        Process* proc = coreAssignments[i];

        if (proc != nullptr) {
            if (proc->IsFinished()) {
                if (proc->GetFinishTime() == 0)
                    proc->SetFinishTime(time(nullptr));
                proc->SetCoreAssigned(-1);
                memoryManager.DeallocateMemory(proc->GetPID(), proc->GetPageTable());
                coreAssignments[i] = nullptr;
                processQuantumCounters.erase(proc);
                continue;
            }

            proc->Execute(i, &memoryManager);
            proc->IncrementExecutionTime();

            if (proc->IsFinished()) {
                if (proc->GetFinishTime() == 0)
                    proc->SetFinishTime(time(nullptr));

                proc->SetState(FINISHED);
                proc->SetCoreAssigned(-1);
                memoryManager.DeallocateMemory(proc->GetPID(), proc->GetPageTable());
                coreAssignments[i] = nullptr;
                processQuantumCounters.erase(proc);

                continue;
            }

            processQuantumCounters[proc]++;

            if (processQuantumCounters[proc] >= quantumCycles) {
                proc->SetState(READY);
                proc->SetCoreAssigned(-1);
                readyQueue.push(proc);
                coreAssignments[i] = nullptr;
                processQuantumCounters.erase(proc);
            }
        }
    }

    // Handle waiting processes
    for (auto proc : allProcesses) {
        if (proc->GetState() == WAITING) {
            proc->DecrementWait();
            if (proc->GetWaitCycles() == 0) {
                proc->SetState(READY);
                readyQueue.push(proc);
            }
        }
    }

    // Assign processes from ready queue to idle cores
    for (int i = 0; i < numCPU; ++i) {
        while (!readyQueue.empty() && readyQueue.front()->IsFinished()) {
            readyQueue.pop();
        }

        if (coreAssignments[i] == nullptr && !readyQueue.empty()) {
            Process* nextProc = readyQueue.front();
            readyQueue.pop();

            if (nextProc->IsFinished()) {
                i--;
                continue;
            }

            nextProc->SetState(RUNNING);
            nextProc->SetCoreAssigned(i);
            coreAssignments[i] = nextProc;
            processQuantumCounters[nextProc] = 0;
        }
    }
}

/* Assigns the next process from ready queue to an available CPU core.
   For both FCFS and Round Robin, processes are taken from the front of the queue (FIFO).
   The difference is FCFS never preempts, while RR preempts after quantum expires. */
void Scheduler::ScheduleNext(int coreId) {
    if (type == FCFS) return;

    if (coreAssignments[coreId] == nullptr && !readyQueue.empty()) {
        Process* nextProc = readyQueue.front();
        readyQueue.pop();

        if (nextProc->IsFinished()) {
            return;
        }

        nextProc->SetState(RUNNING);
        nextProc->SetCoreAssigned(coreId);
        coreAssignments[coreId] = nextProc;

        if (type == ROUND_ROBIN) {
            processQuantumCounters[nextProc] = 0;
        }
    }
}

/* Enables automatic process generation at the configured frequency.
   Called by Console::SchedulerStart() command. */
void Scheduler::Start() {
    isRunning = true;
    cout << "Scheduler started generating processes." << endl;
}

/* Disables automatic process generation but keeps existing processes running.
   Called by Console::SchedulerStop() command. */
void Scheduler::Stop() {
    isRunning = false;
    cout << "Scheduler stopped generating processes." << endl;
}

/* Creates a new process with specified memory size and adds it to the ready queue.
   Used both for manual process creation (screen -s) and automatic generation (scheduler-start).
   Allocates memory through memory manager using demand paging.
   Silently fails if memory allocation fails or process already exists (during auto-generation). */
void Scheduler::CreateNewProcess(const string& name, int memorySize) {
    for (auto existing : allProcesses) {
        if (existing->GetName() == name) {
            // Only show error for manual creation (when scheduler not auto-generating)
            if (!isRunning) {
                cout << "Process " << name << " already exists." << endl;
            }
            return;
        }
    }

    if (memorySize == -1) {
        memorySize = config.minMemPerProc;
    }

    // Check if process memory requirement exceeds total available memory
    // Silently fail during automatic generation to avoid spam
    if (memorySize > config.maxOverallMem) {
        return;
    }

    int numInstructions = minIns + (rand() % (maxIns - minIns + 1));
    Process* proc = new Process(name, currentPID++, numInstructions, delaysPerExec, memorySize);

    // Allocate memory for the process
    if (!memoryManager.AllocateMemory(proc->GetPID(), memorySize, proc->GetPageTable())) {
        delete proc;
        return;
    }

    allProcesses.push_back(proc);
    readyQueue.push(proc);
}

/* Searches for a process by name in the list of all processes.
   Used by Console for screen commands (screen -r <name>). Returns nullptr if not found. */
Process* Scheduler::GetProcess(const string& name) {
    for (auto proc : allProcesses) {
        if (proc->GetName() == name) {
            return proc;
        }
    }
    return nullptr;
}

/* Counts how many CPU cores currently have processes assigned.
   Used for calculating CPU utilization and displaying in screen-ls/report-util. */
int Scheduler::GetCoresUsed() const {
    int count = 0;
    for (const auto& pair : coreAssignments) {
        if (pair.second != nullptr && !pair.second->IsFinished()) {
            count++;
        }
    }
    return count;
}

/* Returns the number of idle CPU cores.
   Used for display in screen-ls and report-util commands. */
int Scheduler::GetCoresAvailable() const {
    return numCPU - GetCoresUsed();
}

/* Calculates CPU utilization as percentage of busy cores over total cores.
   Used for display in screen-ls and report-util commands. */
double Scheduler::GetCPUUtilization() const {
    if (numCPU == 0) return 0.0;
    return (static_cast<double>(GetCoresUsed()) / numCPU) * 100.0;
}

/* Returns all processes that haven't finished execution yet (READY, RUNNING, or WAITING).
   Used by Console::ListScreens() and Console::ReportUtil() to display active processes. */
vector<Process*> Scheduler::GetRunningProcesses() const {
    vector<Process*> running;
    for (auto proc : allProcesses) {
        if (proc != nullptr && !proc->IsFinished()) {
            running.push_back(proc);
        }
    }
    return running;
}

/* Returns all processes that have completed execution.
   Used by Console::ListScreens() and Console::ReportUtil() to display finished processes. */
vector<Process*> Scheduler::GetFinishedProcesses() const {
    vector<Process*> finished;
    for (auto proc : allProcesses) {
        if (proc->IsFinished()) {
            finished.push_back(proc);
        }
    }
    return finished;
}

/* Attempts to assign a specific process to an available CPU core.
   If successful, updates process state and core assignment.
   If no cores are available, adds the process to the ready queue if not already present. */
bool Scheduler::TryAssignProcess(Process* proc) {
    if (proc == nullptr) return false;
    if (proc->IsFinished()) return false;

    while (!readyQueue.empty() && readyQueue.front()->IsFinished()) {
        readyQueue.pop();
    }

    if (proc->GetCoreAssigned() != -1) return true;

    for (int i = 0; i < numCPU; ++i) {
        if (coreAssignments[i] == nullptr) {
            proc->SetState(RUNNING);
            proc->SetCoreAssigned(i);
            coreAssignments[i] = proc;

            if (type == ROUND_ROBIN) {
                processQuantumCounters[proc] = 0;
            }
            return true;
        }
    }

    bool alreadyQueued = false;
    queue<Process*> tmp;
    while (!readyQueue.empty()) {
        Process* p = readyQueue.front();
        readyQueue.pop();
        if (p == proc) alreadyQueued = true;
        tmp.push(p);
    }
    while (!tmp.empty()) {
        readyQueue.push(tmp.front());
        tmp.pop();
    }

    if (!alreadyQueued) {
        proc->SetState(READY);
        readyQueue.push(proc);
    }

    return false;
}