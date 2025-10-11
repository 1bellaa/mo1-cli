#include "process.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

Process::Process(string processName, int processId, int numInstructions, int delaysPerExec)
    : name(processName), pid(processId), state(READY), currentLine(0),
    coreAssigned(-1), waitCycles(0), executionTime(0), delayCounter(0) {

	cout << "Creating process: " << name << " with PID: " << pid << endl;
}

Process::~Process() {
    for (auto instr : instructions) {
        delete instr;
    }
}

void Process::Execute() {
	cout << "Executing process " << name << " (PID: " << pid << ") at line " << currentLine + 1 << "/" << totalLines << endl;
}

bool Process::IsFinished() const {
    return state == FINISHED;
}

void Process::PrintInfo() const {
    cout << "Process: " << name << endl;
    cout << "ID: " << pid << endl;

    if (state == FINISHED) {
        cout << "Finished!" << endl;
    }
    else {
        cout << "Current instruction line: " << currentLine << endl;
        cout << "Lines of code: " << totalLines << endl;
    }

    cout << "\nLogs:" << endl;
    for (const auto& log : outputLog) {
        cout << log << endl;
    }
}

void Process::AddOutput(const string& output) {
	cout << "Process " << name << " output: " << output << endl;
}

uint16_t Process::GetVariable(const string& varName) {
	return variables[varName];
}

void Process::SetVariable(const string& varName, uint16_t value) {
	cout << "Setting variable " << varName << " to " << value << endl;
}