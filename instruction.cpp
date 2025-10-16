/* Barebone instruction implementation */

#include <iostream>
#include <sstream>
#include "instruction.h"
#include "process.h"

using namespace std;

/* PRINT instruction: Logs a message with timestamp and CPU core ID.
   Called by Process::Execute() and stored via Process::AddOutput(). */
PrintInstruction::PrintInstruction(const string& msg, Process* proc)
    : Instruction(proc), message(msg) {
}

/* Executes the PRINT command. Creates formatted timestamp and core info.
   Connects with Process::AddOutput() to record output into process logs. */
void PrintInstruction::Execute() {
    time_t now = time(nullptr);
    tm local_tm;
    localtime_s(&local_tm, &now);
    char buf[64];
    strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S %p)", &local_tm);

    int core = process->GetCoreAssigned();
    string logEntry = string(buf) + " Core:" + to_string(core) + " \"" + message + "\"";
    process->AddOutput(logEntry);
}

/* DECLARE instruction: Creates a variable and assigns an initial value.
   Used to simulate variable declaration in the process’s memory. */
DeclareInstruction::DeclareInstruction(const string& var, uint16_t val, Process* proc)
    : Instruction(proc), varName(var), value(val) {
}

/* Executes DECLARE by setting the variable’s value.
   Connects to Process::SetVariable(). */
void DeclareInstruction::Execute() {
    process->SetVariable(varName, value);
}

/* ADD instruction: Performs integer addition between variable and/or constant values.
   Logic clamps result to uint16_t range to simulate 16-bit arithmetic. */
AddInstruction::AddInstruction(const string& result, const string& op1, uint16_t op2, Process* proc)
    : Instruction(proc), resultVar(result), operand1(op1), operand2(op2), useVariable(false) {
}

/* Executes ADD instruction, reads operand, computes sum, and saves result. */
void AddInstruction::Execute() {
    uint16_t val1 = process->GetVariable(operand1);
    uint32_t result = static_cast<uint32_t>(val1) + static_cast<uint32_t>(operand2);

    if (result > UINT16_MAX) {
        result = UINT16_MAX;
    }

    process->SetVariable(resultVar, static_cast<uint16_t>(result));
}

/* SUBTRACT instruction: Performs integer subtraction.
   Clamps negative results to zero (unsigned integer behavior). */
SubtractInstruction::SubtractInstruction(const string& result, const string& op1, uint16_t op2, Process* proc)
    : Instruction(proc), resultVar(result), operand1(op1), operand2(op2), useVariable(false) {
}

/* Executes SUBTRACT instruction by computing op1 - op2 and updating target variable. */
void SubtractInstruction::Execute() {
    uint16_t val1 = process->GetVariable(operand1);
    int32_t result = static_cast<int32_t>(val1) - static_cast<int32_t>(operand2);

    if (result < 0) {
        result = 0;
    }

    process->SetVariable(resultVar, static_cast<uint16_t>(result));
}

/* SLEEP instruction: Puts process into WAITING state for specified CPU cycles.
   Used to simulate blocking or yielding execution. */
SleepInstruction::SleepInstruction(uint8_t cpuCycles, Process* proc)
    : Instruction(proc), cycles(cpuCycles) {
}

/* Executes SLEEP by setting process state and defining wait cycles.
   Scheduler later decrements wait time during Tick(). */
void SleepInstruction::Execute() {
    process->SetState(WAITING);
}

/* FOR loop instruction: Contains nested instructions repeated multiple times.
   Allows grouped operations to be executed sequentially. */
ForLoopInstruction::ForLoopInstruction(const vector<Instruction*>& instructions, int numRepeats, Process* proc)
    : Instruction(proc), loopInstructions(instructions), repeats(numRepeats),
    currentIteration(0), currentInstructionIndex(0) {
}

/* Destructor: Cleans up dynamically allocated sub-instructions within the loop. */
ForLoopInstruction::~ForLoopInstruction() {
    for (auto instr : loopInstructions) {
        delete instr;
    }
}

/* Executes FOR loop by repeatedly executing each inner instruction for N iterations.
   Connects to other Instruction types and simulates simple looping logic. */
void ForLoopInstruction::Execute() {
    if (currentIteration < repeats) {
        if (currentInstructionIndex < loopInstructions.size()) {
            loopInstructions[currentInstructionIndex]->Execute();
            currentInstructionIndex++;

            if (currentInstructionIndex >= loopInstructions.size()) {
                currentInstructionIndex = 0;
                currentIteration++;
            }
        }
    }
    /*if (currentIteration < repeats) {
        loopInstructions[currentInstructionIndex]->Execute();
        currentInstructionIndex++;

        if (currentInstructionIndex >= loopInstructions.size()) {
            currentInstructionIndex = 0;
            currentIteration++;
        }
    }*/
}