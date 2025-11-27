#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class Process;
class MemoryManager;

class Instruction {
protected:
    Process* process;

public:
    Instruction(Process* proc) : process(proc) {}
    virtual ~Instruction() {}
    virtual void Execute(MemoryManager* memMgr = nullptr) = 0;
};

class PrintInstruction : public Instruction {
private:
    string message;

public:
    PrintInstruction(const string& msg, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

class DeclareInstruction : public Instruction {
private:
    string varName;
    uint16_t value;

public:
    DeclareInstruction(const string& var, uint16_t val, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

class AddInstruction : public Instruction {
private:
    string resultVar;
    string operand1;
    uint16_t operand2;
    bool useVariable;

public:
    AddInstruction(const string& result, const string& op1, uint16_t op2, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

class SubtractInstruction : public Instruction {
private:
    string resultVar;
    string operand1;
    uint16_t operand2;
    bool useVariable;

public:
    SubtractInstruction(const string& result, const string& op1, uint16_t op2, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

class SleepInstruction : public Instruction {
private:
    uint8_t cycles;

public:
    SleepInstruction(uint8_t cpuCycles, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

class ForLoopInstruction : public Instruction {
private:
    vector<Instruction*> loopInstructions;
    int repeats;
    int currentIteration;
    int currentInstructionIndex;

public:
    ForLoopInstruction(const vector<Instruction*>& instructions, int numRepeats, Process* proc);
    ~ForLoopInstruction();
    void Execute(MemoryManager* memMgr = nullptr) override;
};

// New MO2 instructions
class ReadInstruction : public Instruction {
private:
    string varName;
    uint32_t memoryAddress;

public:
    ReadInstruction(const string& var, uint32_t addr, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

class WriteInstruction : public Instruction {
private:
    uint32_t memoryAddress;
    uint16_t value;
    string varName;  // If writing from variable
    bool useVariable;

public:
    WriteInstruction(uint32_t addr, uint16_t val, Process* proc);
    WriteInstruction(uint32_t addr, const string& var, Process* proc);
    void Execute(MemoryManager* memMgr = nullptr) override;
};

#endif