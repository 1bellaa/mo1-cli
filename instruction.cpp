#include <iostream>
#include <sstream>
#include "instruction.h"
#include "process.h"

using namespace std;

PrintInstruction::PrintInstruction(const string& msg, Process* proc)
    : Instruction(proc), message(msg) {
}

void PrintInstruction::Execute() {
	cout << "Process " << process->GetName() << " printing message: " << message << endl;
}

DeclareInstruction::DeclareInstruction(const string& var, uint16_t val, Process* proc)
    : Instruction(proc), varName(var), value(val) {
}

void DeclareInstruction::Execute() {
	cout << "Process " << process->GetName() << " declaring variable " << varName << " with value " << value << endl;
}

AddInstruction::AddInstruction(const string& result, const string& op1, uint16_t op2, Process* proc)
    : Instruction(proc), resultVar(result), operand1(op1), operand2(op2), useVariable(false) {
}

void AddInstruction::Execute() {
	cout << "Process " << process->GetName() << " adding " << operand2 << " to variable " << operand1 << " and storing in " << resultVar << endl;
}

SubtractInstruction::SubtractInstruction(const string& result, const string& op1, uint16_t op2, Process* proc)
    : Instruction(proc), resultVar(result), operand1(op1), operand2(op2), useVariable(false) {
}

void SubtractInstruction::Execute() {
	cout << "Process " << process->GetName() << " subtracting " << operand2 << " from variable " << operand1 << " and storing in " << resultVar << endl;
}

SleepInstruction::SleepInstruction(uint8_t cpuCycles, Process* proc)
    : Instruction(proc), cycles(cpuCycles) {
}

void SleepInstruction::Execute() {
	cout << "Process " << process->GetName() << " sleeping for " << static_cast<int>(cycles) << " CPU cycles." << endl;
}

ForLoopInstruction::ForLoopInstruction(const vector<Instruction*>& instructions, int numRepeats, Process* proc)
    : Instruction(proc), loopInstructions(instructions), repeats(numRepeats),
    currentIteration(0), currentInstructionIndex(0) {
}

ForLoopInstruction::~ForLoopInstruction() {
	cout << "ForLoopInstruction" << endl;
}

void ForLoopInstruction::Execute() {
	cout << "Executing For Loop iteration " << currentIteration + 1 << "/" << repeats << endl;
}