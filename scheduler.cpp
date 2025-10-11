#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include "scheduler.h"

using namespace std;

Scheduler::Scheduler()
    : type(FCFS), numCPU(1), quantumCycles(5), batchProcessFreq(1),
    minIns(100), maxIns(1000), delaysPerExec(0),
    currentPID(1), cpuTicks(0), processCounter(1), isRunning(false) {
    srand(time(NULL));
}

Scheduler::~Scheduler() {
    for (auto proc : allProcesses) {
        delete proc;
    }
}

void Scheduler::Initialize(const string& configFile) {
    LoadConfig(configFile);

    // Initialize core assignments
    for (int i = 0; i < numCPU; i++) {
        coreAssignments[i] = nullptr;
    }

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

void Scheduler::LoadConfig(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open config file " << filename << endl;
        return;
    }

	cout << "Loading configuration from " << filename << "..." << endl;

    file.close();
}

void Scheduler::Tick() {
	cout << "Tick " << cpuTicks << endl;
}

void Scheduler::ScheduleNext(int coreId) {
	cout << "Scheduling next process on core " << coreId << endl;
}

void Scheduler::Start() {
    isRunning = true;
    cout << "Scheduler started generating processes." << endl;
}

void Scheduler::Stop() {
    isRunning = false;
    cout << "Scheduler stopped generating processes." << endl;
}

void Scheduler::CreateProcess(const string& name) {
	cout << "Creating process: " << name << endl;
}

Process* Scheduler::GetProcess(const string& name) {
	return nullptr;
}

int Scheduler::GetCoresUsed() const {
	return numCPU - GetCoresAvailable();
}

int Scheduler::GetCoresAvailable() const {
	return numCPU - GetCoresUsed();
}

double Scheduler::GetCPUUtilization() const {
	return (static_cast<double>(GetCoresUsed()) / numCPU) * 100.0;
}

vector<Process*> Scheduler::GetRunningProcesses() const {
	return vector<Process*>();
}

vector<Process*> Scheduler::GetFinishedProcesses() const {
	return vector<Process*>();
}