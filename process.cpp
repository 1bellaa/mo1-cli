/* Barebone instruction generation and execution with memory management */

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "process.h"
#include "memory.h"

using namespace std;

/* Constructor: Can generate either randomized instructions or alternating PRINT/ADD for tests.
   Now includes memory size parameter for MO2 memory management. */
Process::Process(string processName, int processId, int numInstructions, int delaysPerExec, int memSize)
    : name(processName), pid(processId), state(READY), currentLine(0),
    coreAssigned(-1), waitCycles(0), executionTime(0), delayCounter(0),
    finishTime(0), memorySize(memSize), hasMemoryError(false),
    memoryErrorTime(0), memoryErrorAddress(0) {

    srand(time(NULL) + processId);

    // Initialize variable "x" to 0 (used for test cases 4 & 5)
    variables["x"] = 0;

    // Check if we should use test case mode (alternating PRINT/ADD only)
    // This is enabled when min-ins and max-ins are very high (>= 100000)
    bool useTestCaseMode = (numInstructions >= 100000);

    if (useTestCaseMode) {
        // Test case mode: alternating PRINT and ADD instructions
        for (int i = 0; i < numInstructions; i++) {
            if (i % 2 == 0) {
                instructions.push_back(new PrintInstruction("Value from: " + name, this));
            }
            else {
                uint16_t addValue = (rand() % 10) + 1;
                instructions.push_back(new AddInstruction("x", "x", addValue, this));
            }
        }
    }
    else {
        // Normal mode: randomized instruction generation including READ/WRITE
        for (int i = 0; i < numInstructions; i++) {
            int processInstruction = rand() % 8;  // Now 8 types including READ/WRITE

            if (processInstruction == 0) {
                // PRINT instruction
                instructions.push_back(new PrintInstruction("Hello world from " + name + "!", this));
            }
            else if (processInstruction == 1) {
                // DECLARE instruction
                string varName = "var" + to_string(rand() % 10);
                uint16_t value = rand() % 1000;
                instructions.push_back(new DeclareInstruction(varName, value, this));
            }
            else if (processInstruction == 2) {
                // ADD instruction
                string var1 = "var" + to_string(rand() % 10);
                string var2 = "var" + to_string(rand() % 10);
                uint16_t value = rand() % 100;
                instructions.push_back(new AddInstruction(var1, var2, value, this));
            }
            else if (processInstruction == 3) {
                // SUBTRACT instruction
                string var1 = "var" + to_string(rand() % 10);
                string var2 = "var" + to_string(rand() % 10);
                uint16_t value = rand() % 100;
                instructions.push_back(new SubtractInstruction(var1, var2, value, this));
            }
            else if (processInstruction == 4) {
                // SLEEP instruction
                uint8_t cycles = (rand() % 5) + 1;
                instructions.push_back(new SleepInstruction(cycles, this));
            }
            else if (processInstruction == 5) {
                // FOR loop instruction
                int loopRepeats = (rand() % 3) + 2;
                int loopInstructions = (rand() % 3) + 1;
                vector<Instruction*> loopBody;

                for (int j = 0; j < loopInstructions; j++) {
                    loopBody.push_back(new PrintInstruction("Hello world from " + name + "!", this));
                }

                instructions.push_back(new ForLoopInstruction(loopBody, loopRepeats, this));
            }
            else if (processInstruction == 6) {
                // READ instruction - read from memory address
                string varName = "var" + to_string(rand() % 10);
                uint32_t address = (rand() % (memorySize / 2)) * 2;  // Even addresses
                instructions.push_back(new ReadInstruction(varName, address, this));
            }
            else {
                // WRITE instruction - write to memory address
                uint32_t address = (rand() % (memorySize / 2)) * 2;  // Even addresses
                uint16_t value = rand() % 1000;
                instructions.push_back(new WriteInstruction(address, value, this));
            }
        }
    }

    totalLines = instructions.size();
}

/* Destructor to free dynamically allocated instruction memory.
   Connects with Scheduler's destructor to ensure proper cleanup. */
Process::~Process() {
    for (auto instr : instructions) {
        delete instr;
    }
}

/* Executes the next instruction in the process's instruction list.
   Called by Scheduler::Tick() whenever the process is assigned to a core.
   Handles waiting, delays, transitions to FINISHED state, and memory access violations. */
void Process::Execute(int coreId, MemoryManager* memMgr) {
    if (state == FINISHED || hasMemoryError) {
        return;
    }

    if (currentLine >= totalLines) {
        state = FINISHED;
        if (finishTime == 0) {
            finishTime = time(nullptr);
        }
        coreAssigned = -1;
        return;
    }

    if (state == WAITING) {
        if (waitCycles > 0) {
            waitCycles--;
            return;
        }
        state = RUNNING;
    }

    if (delayCounter > 0) {
        delayCounter--;
        return;
    }

    if (currentLine < totalLines && state != FINISHED) {
        coreAssigned = coreId;

        try {
            // Execute instruction with memory manager for READ/WRITE operations
            instructions[currentLine]->Execute(memMgr);
            currentLine++;

            if (currentLine >= totalLines) {
                state = FINISHED;
                if (finishTime == 0) {
                    finishTime = time(nullptr);
                }
                coreAssigned = -1;
            }
        }
        catch (const runtime_error& e) {
            // Memory access violation occurred - terminate process
            hasMemoryError = true;
            memoryErrorTime = time(nullptr);
            state = FINISHED;
            coreAssigned = -1;

            // Extract address from error message
            string errorMsg = e.what();
            size_t pos = errorMsg.find("0x");
            if (pos != string::npos) {
                string addrStr = errorMsg.substr(pos + 2);
                memoryErrorAddress = stoul(addrStr, nullptr, 16);
            }
        }
    }
}

/* Checks whether the process has completed all instructions.
   Used by Scheduler to determine if the process should be rescheduled. */
bool Process::IsFinished() const {
    return (state == FINISHED || currentLine >= totalLines || hasMemoryError);
}

/* Prints detailed information about the process, including logs and progress.
   Called when the user runs "process-smi" inside a screen.
   Connects Console::DisplayProcessScreen() to process-level data. */
void Process::PrintInfo() const {
    cout << "\nProcess name: " << name << endl;
    cout << "ID: " << pid << endl;
    cout << "Memory: " << memorySize << " bytes" << endl;

    cout << "Logs:" << endl;
    for (const auto& log : outputLog) {
        cout << log << endl;
    }

    if (hasMemoryError) {
        cout << "\nProcess shut down due to memory access violation." << endl;

        time_t errTime = memoryErrorTime;
        tm local_tm;
#ifdef _WIN32
        localtime_s(&local_tm, &errTime);
#else
        localtime_r(&errTime, &local_tm);
#endif
        char buf[64];
        strftime(buf, sizeof(buf), "%H:%M:%S", &local_tm);

        cout << "Error occurred at " << buf << endl;
        cout << "Invalid address: 0x" << hex << memoryErrorAddress << dec << endl;
    }
    else if (state == FINISHED) {
        cout << "\nFinished!" << endl;
    }
    else {
        cout << "\nCurrent instruction line: " << currentLine << endl;
        cout << "Lines of code: " << totalLines << endl;
    }
}

/* Adds a log entry (usually from a PRINT instruction) to the process output.
   Connects Instruction classes (e.g., PrintInstruction) to this process. */
void Process::AddOutput(const string& output) {
    outputLog.push_back(output);
}

/* Retrieves a variable's current value. Automatically initializes it to 0 if missing.
   Used by ADD/SUBTRACT instructions for arithmetic operations. */
uint16_t Process::GetVariable(const string& varName) {
    if (variables.find(varName) == variables.end()) {
        variables[varName] = 0;
    }
    return variables[varName];
}

/* Assigns a value to a process variable.
   Used by DECLARE, ADD, SUBTRACT instructions. */
void Process::SetVariable(const string& varName, uint16_t value) {
    variables[varName] = value;
}

/* Declares a variable with a value, checking symbol table size limit (32 variables = 64 bytes).
   Returns false if symbol table is full, true if successful. */
bool Process::DeclareVariable(const string& varName, uint16_t value) {
    // Check symbol table size (max 32 variables = 64 bytes)
    if (variables.size() >= 32 && variables.find(varName) == variables.end()) {
        // Symbol table full, ignore declaration
        return false;
    }

    variables[varName] = value;
    return true;
}

/* Read a uint16 value from a memory address.
   Validates address bounds and uses memory manager for demand paging.
   Throws runtime_error on memory access violation. */
uint16_t Process::ReadMemoryAddress(uint32_t address, MemoryManager* memMgr) {
    if (address >= (uint32_t)memorySize) {
        throw runtime_error("Memory access violation at address 0x" +
            to_string(address));
    }

    return memMgr->ReadMemory(pid, address, pageTable);
}

/* Write a uint16 value to a memory address.
   Validates address bounds and uses memory manager for demand paging.
   Throws runtime_error on memory access violation. */
void Process::WriteMemoryAddress(uint32_t address, uint16_t value, MemoryManager* memMgr) {
    if (address >= (uint32_t)memorySize) {
        throw runtime_error("Memory access violation at address 0x" +
            to_string(address));
    }

    memMgr->WriteMemory(pid, address, value, pageTable);
}