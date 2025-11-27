/* Barebone instruction implementation with memory management operations */

#include <iostream>
#include <sstream>
#include "instruction.h"
#include "process.h"
#include "memory.h"

using namespace std;

/* PRINT instruction: Logs a message with timestamp and CPU core ID.
   Called by Process::Execute() and stored via Process::AddOutput().
   Now accepts MemoryManager parameter for consistency with other instructions. */
PrintInstruction::PrintInstruction(const string& msg, Process* proc)
    : Instruction(proc), message(msg) {
}

/* Executes the PRINT command. Creates formatted timestamp and core info.
   Prints the value of variable "x" in the message.
   Connects with Process::AddOutput() to record output into process logs. */
void PrintInstruction::Execute(MemoryManager* memMgr) {
    time_t now = time(nullptr);
    tm local_tm;
    localtime_s(&local_tm, &now);
    char buf[64];
    strftime(buf, sizeof(buf), "(%m/%d/%Y %I:%M:%S %p)", &local_tm);

    int core = process->GetCoreAssigned();
    uint16_t xValue = process->GetVariable("x");

    string logEntry = string(buf) + " Core:" + to_string(core) + " \"" + message + " " + to_string(xValue) + "\"";
    process->AddOutput(logEntry);
}

/* DECLARE instruction: Creates a variable and assigns an initial value.
   Used to simulate variable declaration in the process's memory.
   Now uses Process::DeclareVariable() which enforces 32-variable limit. */
DeclareInstruction::DeclareInstruction(const string& var, uint16_t val, Process* proc)
    : Instruction(proc), varName(var), value(val) {
}

/* Executes DECLARE by setting the variable's value.
   Connects to Process::DeclareVariable() which checks symbol table size limit. */
void DeclareInstruction::Execute(MemoryManager* memMgr) {
    process->DeclareVariable(varName, value);
}

/* ADD instruction: Performs integer addition between variable and/or constant values.
   Logic clamps result to uint16_t range to simulate 16-bit arithmetic. */
AddInstruction::AddInstruction(const string& result, const string& op1, uint16_t op2, Process* proc)
    : Instruction(proc), resultVar(result), operand1(op1), operand2(op2), useVariable(false) {
}

/* Executes ADD instruction, reads operand, computes sum, and saves result. */
void AddInstruction::Execute(MemoryManager* memMgr) {
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
void SubtractInstruction::Execute(MemoryManager* memMgr) {
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

/* Executes SLEEP by setting process state to WAITING.
   Scheduler later decrements wait time during Tick(). */
void SleepInstruction::Execute(MemoryManager* memMgr) {
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
   Connects to other Instruction types and simulates simple looping logic.
   Now passes MemoryManager to nested instruction execution. */
void ForLoopInstruction::Execute(MemoryManager* memMgr) {
    if (currentIteration < repeats) {
        if (currentInstructionIndex < loopInstructions.size()) {
            loopInstructions[currentInstructionIndex]->Execute(memMgr);
            currentInstructionIndex++;

            if (currentInstructionIndex >= loopInstructions.size()) {
                currentInstructionIndex = 0;
                currentIteration++;
            }
        }
    }
}

/* READ instruction: Reads a uint16 value from a memory address into a variable.
   New for MO2: Simulates memory access with demand paging.
   Throws runtime_error on memory access violation. */
ReadInstruction::ReadInstruction(const string& var, uint32_t addr, Process* proc)
    : Instruction(proc), varName(var), memoryAddress(addr) {
}

/* Executes READ by calling Process::ReadMemoryAddress() which handles
   page faults through the memory manager. Stores result in target variable.
   Throws runtime_error if address is invalid (caught by Process::Execute). */
void ReadInstruction::Execute(MemoryManager* memMgr) {
    if (memMgr == nullptr) {
        cerr << "Error: Memory manager not available" << endl;
        return;
    }

    try {
        uint16_t value = process->ReadMemoryAddress(memoryAddress, memMgr);
        process->SetVariable(varName, value);
    }
    catch (const runtime_error& e) {
        // Memory access violation handled in process
        throw;
    }
}

/* WRITE instruction: Writes a uint16 value (constant or variable) to a memory address.
   New for MO2: Simulates memory access with demand paging.
   Throws runtime_error on memory access violation. */
WriteInstruction::WriteInstruction(uint32_t addr, uint16_t val, Process* proc)
    : Instruction(proc), memoryAddress(addr), value(val), useVariable(false) {
}

/* Constructor overload: WRITE instruction using a variable as the value source. */
WriteInstruction::WriteInstruction(uint32_t addr, const string& var, Process* proc)
    : Instruction(proc), memoryAddress(addr), varName(var), value(0), useVariable(true) {
}

/* Executes WRITE by calling Process::WriteMemoryAddress() which handles
   page faults through the memory manager. Can write constant or variable value.
   Throws runtime_error if address is invalid (caught by Process::Execute). */
void WriteInstruction::Execute(MemoryManager* memMgr) {
    if (memMgr == nullptr) {
        cerr << "Error: Memory manager not available" << endl;
        return;
    }

    try {
        uint16_t writeValue = useVariable ? process->GetVariable(varName) : value;
        process->WriteMemoryAddress(memoryAddress, writeValue, memMgr);
    }
    catch (const runtime_error& e) {
        // Memory access violation handled in process
        throw;
    }
}