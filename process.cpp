/* Barebone instruction generation and execution */

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include "process.h"

using namespace std;

/* Constructor: Builds a new process instance with randomized instructions.
   Uses rand() to randomly select between instruction types and connects 
   directly with Scheduler::CreateNewProcess() which calls this constructor. */
Process::Process(string processName, int processId, int numInstructions, int delaysPerExec)
    : name(processName), pid(processId), state(READY), currentLine(0),
    coreAssigned(-1), waitCycles(0), executionTime(0), delayCounter(0), finishTime(0) {

    srand(time(NULL) + processId);

    for (int i = 0; i < numInstructions; i++) {
        int processInstruction = rand() % 6;
        
		/* ONLY FOR CHECKING, use with Console::DisplayProcessScreen()'s cls
        cout << name << " Instruction " << i
             << " type: " << processInstruction << endl;*/

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
        else {
            // FOR loop instruction
            int loopRepeats = (rand() % 3) + 2;
            int loopInstructions = (rand() % 3) + 1;
            vector<Instruction*> loopBody;

            for (int j = 0; j < loopInstructions; j++) {
                loopBody.push_back(new PrintInstruction("Hello world from " + name + "!", this));
            }

            instructions.push_back(new ForLoopInstruction(loopBody, loopRepeats, this));
        }
    }

    totalLines = instructions.size();
}

/* Destructor to free dynamically allocated instruction memory.
   Connects with Scheduler’s destructor to ensure proper cleanup. */
Process::~Process() {
    for (auto instr : instructions) {
        delete instr;
    }
}

/* Executes the next instruction in the process’s instruction list.
   Called by Scheduler::Tick() whenever the process is assigned to a core.
   Handles waiting, delays, and transitions to FINISHED state. */
void Process::Execute(int coreId) {
    if (state == FINISHED || currentLine >= totalLines) {
        state = FINISHED;
        
        // Record the exact time of completion if not already set
        if (finishTime == 0) {
            finishTime = time(nullptr);
        }
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

    if (currentLine < totalLines) {
        coreAssigned = coreId; // temp
        instructions[currentLine]->Execute();
        currentLine++;

        if (currentLine >= totalLines) {
            state = FINISHED;
        }
    }
}

/* Checks whether the process has completed all instructions.
   Used by Scheduler to determine if the process should be rescheduled. */
bool Process::IsFinished() const {
    return state == FINISHED;
}

/* Prints detailed information about the process, including logs and progress.
   Called when the user runs "process-smi" inside a screen.
   Connects Console::DisplayProcessScreen() to process-level data. */
void Process::PrintInfo() const {
    cout << "\nProcess name: " << name << endl;
    cout << "ID: " << pid << endl;
    
    cout << "Logs:" << endl;
    for (const auto& log : outputLog) {
        cout << log << endl;
    }

    if (state == FINISHED) {
        cout << "\nFinished!" << endl;
    } else {
        cout << "\nCurrent instruction line: " << currentLine << endl;
        cout << "Lines of code: " << totalLines << endl;
    }
}

/* Adds a log entry (usually from a PRINT instruction) to the process output.
   Connects Instruction classes (e.g., PrintInstruction) to this process. */
void Process::AddOutput(const string& output) {
    outputLog.push_back(output);
}

/* Retrieves a variable’s current value. Automatically initializes it to 0 if missing.
   Used by ADD/SUBTRACT instructions for arithmetic operations. */
uint16_t Process::GetVariable(const string& varName) {
    if (variables.find(varName) == variables.end()) {
        variables[varName] = 0;
    }
    return variables[varName];
}

/* Assigns a value to a process variable. */
void Process::SetVariable(const string& varName, uint16_t value) {
    variables[varName] = value;
}