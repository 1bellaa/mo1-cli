/* Process scheduling logic and scheduling algorithm */

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include "scheduler.h"

using namespace std;

/* Constructor initializes the scheduler with default configuration values 
   if config.txt is not found. */
Scheduler::Scheduler(): currentPID(1), cpuTicks(0), 
                        processCounter(1), isRunning(false) {
    config.numCPU = 4;
    config.type = ROUND_ROBIN;
    config.quantumCycles = 5;
    config.batchProcessFreq = 1;
    config.minIns = 1000;
    config.maxIns = 2000;
    config.delaysPerExec = 0;

    numCPU = config.numCPU;
    type = config.type;
    quantumCycles = config.quantumCycles;
    batchProcessFreq = config.batchProcessFreq;
    minIns = config.minIns;
    maxIns = config.maxIns;
    delaysPerExec = config.delaysPerExec;

    //srand(time(NULL));
}

/* Destructor cleans up all dynamically allocated processes.
   Called when the console terminates, ensuring no memory leaks. */
Scheduler::~Scheduler() {
    for (auto proc : allProcesses) {
        delete proc;
    }
}

/* Loads configuration from file and initializes CPU cores.
   This is called by Console::Initialize() to set up the scheduling environment. */
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

    /* To comment out, hindi yata kasama sa specs na dapat ipakita, for checking lang */
    cout << "System initialized with:" << endl;
    cout << "CPUs: " << numCPU << endl;
    cout << "Scheduler: " << (type == FCFS ? "FCFS" : "Round Robin") << endl;
    if (type == ROUND_ROBIN) {
        cout << "Quantum cycles: " << quantumCycles << endl;
    }
    cout << "Batch process frequency: " << batchProcessFreq << endl;
    cout << "Min instructions: " << minIns << endl;
    cout << "Max instructions: " << maxIns << endl;
    cout << "Delays per exec: " << delaysPerExec << endl;
}

/* Reads scheduler configuration from config.txt, setting algorithm type and process parameters.
   Falls back to default values if file cannot be opened. */
void Scheduler::LoadConfig(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        //cerr << "Error: Could not open config file " << filename << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string key, value;

        if (iss >> key >> value) {
            // Remove surrounding quotes if present
            if (!value.empty() && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }

            if (key == "num-cpu") {
                config.numCPU = stoi(value);
            }
            else if (key == "scheduler") {
                if (value == "fcfs") config.type = FCFS;
                else if (value == "rr") config.type = ROUND_ROBIN;
            }
            else if (key == "quantum-cycles") {
                config.quantumCycles = stoi(value);
            }
            else if (key == "batch-process-freq") {
                config.batchProcessFreq = stoi(value);
            }
            else if (key == "min-ins") {
                config.minIns = stoi(value);
            }
            else if (key == "max-ins") {
                config.maxIns = stoi(value);
            }
            else if (key == "delays-per-exec") {
                config.delaysPerExec = stoi(value);
            }
        }
    }

    file.close();
}

/* Executes one CPU cycle across all cores.
   Generates new processes based on frequency, executes running processes,
   handles preemption for Round Robin, and manages process state transitions. */
void Scheduler::Tick() {
    cpuTicks++;

    /* For FCFS logic */
    if (type == FCFS) {
        // Generate new processes periodically if running
        if (isRunning && cpuTicks % batchProcessFreq == 0) {
            string processName = "process" + to_string(processCounter++);
            CreateNewProcess(processName);
        }

        // Assign ready processes to any idle cores
        for (int i = 0; i < numCPU; ++i) {
            if (coreAssignments[i] == nullptr && !readyQueue.empty()) {
                Process* nextProc = readyQueue.front();
                readyQueue.pop();

                nextProc->SetState(RUNNING);
                nextProc->SetCoreAssigned(i);
                coreAssignments[i] = nextProc;
            }
        }

        // Execute one instruction for each running process
        for (int i = 0; i < numCPU; ++i) {
            Process* proc = coreAssignments[i];
            if (proc != nullptr) {
                proc->Execute(i);
                proc->IncrementExecutionTime();

                if (proc->IsFinished()) {
                    if (proc->GetFinishTime() == 0)
                        proc->SetFinishTime(time(nullptr));

                    proc->SetCoreAssigned(-1);
                    coreAssignments[i] = nullptr;
                }
            }
        }

        // Handle waiting processes (sleep)
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
    /* For RR logic */
    if (isRunning && cpuTicks % batchProcessFreq == 0) {
        string processName = "process" + to_string(processCounter++);
        CreateNewProcess(processName);
    }

    for (int i = 0; i < numCPU; ++i) {
        Process* proc = coreAssignments[i];

        if (proc != nullptr) {
            // Execute process
            proc->Execute(i);
            proc->IncrementExecutionTime();

            // Handle quantum expiration
            processQuantumCounters[proc]++;

            if (processQuantumCounters[proc] >= quantumCycles && !proc->IsFinished()) {
                // Preempt process
                proc->SetState(READY);
                //proc->SetCoreAssigned(-1); // temporarily unassigned
                readyQueue.push(proc);
                coreAssignments[i] = nullptr;
                processQuantumCounters[proc] = 0;
            }

            // Handle process completion
            if (proc->IsFinished()) {
                if (proc->GetFinishTime() == 0)
                    proc->SetFinishTime(time(nullptr));
                proc->SetCoreAssigned(-1);
                coreAssignments[i] = nullptr;
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

    // Prevent lingering -1 assignments
    for (int i = 0; i < numCPU; ++i) {
        if (coreAssignments[i] == nullptr && !readyQueue.empty()) {
            Process* nextProc = readyQueue.front();
            readyQueue.pop();

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

/* Creates a new process with random instruction count and adds it to the ready queue.
   Used both for manual process creation (screen -s) and automatic generation (scheduler-start). */
void Scheduler::CreateNewProcess(const string& name) {
    // Check for existing process with the same name
    for (auto existing : allProcesses) {
        if (existing->GetName() == name) {
            // Already exists, skip creation
            //cout << "Skipping duplicate process: " << name << endl;
            return;
        }
    }
    
    int numInstructions = minIns + (rand() % (maxIns - minIns + 1));
    Process* proc = new Process(name, currentPID++, numInstructions, delaysPerExec);

    allProcesses.push_back(proc);
    readyQueue.push(proc);

    // cout << "Process " << name << " created with " << numInstructions << " instructions." << endl;
    //processLogs.push_back("Process " + name + " created with " + to_string(numInstructions) + " instructions."); // to avoid console flooding of threads
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
        if (pair.second != nullptr) {
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
        if (!proc->IsFinished()) {
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

bool Scheduler::TryAssignProcess(Process* proc) {
    if (proc == nullptr) return false;
    if (proc->IsFinished()) return false;

    // If already assigned, nothing to do.
    if (proc->GetCoreAssigned() != -1) return true;

    // look for an idle core
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
    std::queue<Process*> tmp;
    while (!readyQueue.empty()) {
        Process* p = readyQueue.front(); readyQueue.pop();
        if (p == proc) alreadyQueued = true;
        tmp.push(p);
    }
    while (!tmp.empty()) { readyQueue.push(tmp.front()); tmp.pop(); }

    if (!alreadyQueued) readyQueue.push(proc);

    return false;
}
