/* CLI interface + I/O commands */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#include <windows.h>
#include "console.h"
//#include "scheduler.h"
#include "memoryallocator.h"
#include "memoryvisual.h"
//#include "instruction.h"
//#include "process.h"

using namespace std;

extern MemoryAllocator memAlloc;
extern MemoryVisual* memVis;

/* Constructor initializes internal state of the Console.
   Connects to the Scheduler and tick thread which simulate CPU ticks. */
Console::Console() : initialized(false), tickThread(nullptr), shouldRunTicks(false) {}

/* Destructor stops the tick thread and cleans up memory.
   Ensures the background CPU ticking thread ends gracefully. */
Console::~Console() {
    if (tickThread != nullptr) {
        shouldRunTicks = false;
        if (tickThread->joinable()) {
            tickThread->join();
        }
        delete tickThread;
    }
}

/* Initializes the system by loading configuration and starting CPU ticks.
   Connects to Scheduler::Initialize() to load config.txt. */
void Console::Initialize() {
    scheduler.Initialize("config.txt");
    initialized = true;

    /*size_t total = 65536;
    size_t framesz = 256;
    memAlloc.Initialize(total, framesz);*/

    shouldRunTicks = true;
    tickThread = new thread(&Console::TickLoop, this);

    cout << "Console initialized successfully." << endl; // will comment out after
}

/* Simulates the CPU ticking mechanism in real-time.
   Continuously calls Scheduler::Tick() to update process states. */
void Console::TickLoop() {
    while (shouldRunTicks) {
        scheduler.Tick();
        this_thread::sleep_for(chrono::milliseconds(100)); // ms per tick, idk if correct tho
    }
}

/* Utility to format timestamps consistently. */
static string FormatTimestamp(time_t t) {
    char buf[64];
    tm local_tm;
    localtime_s(&local_tm, &t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S %p", &local_tm);
    return string(buf);
}

/* Creates a new process “screen” for user interaction.
   Connects to Scheduler::CreateNewProcess() to spawn a process. */
void Console::CreateScreen(const string& processName) {
    Process* proc = scheduler.GetProcess(processName);
    
    istringstream iss(processName);
    string name;
    size_t memSize = 0;
    if (!(iss >> name >> memSize)) {
        cout << "Invalid screen -s usage. Format: screen -s <process_name> <process_memory_size>" << endl;
        return;
    }

    Process* proc = scheduler.GetProcess(name);
    if (proc != nullptr) {
        cout << "Process " << processName << " already exists." << endl;
        return;
    }

    /* validate memSize : power of two, between 64 and 65536 bytes(2^6...2^16) */
    if (memSize < 64 || memSize > 65536) {
        cout << "invalid memory allocation" << endl;
        return;
    }
    
    auto isPow2 = [](size_t x) { return x && !(x & (x - 1)); };
    if (!isPow2(memSize)) {
        cout << "invalid memory allocation" << endl;
        return;
    }

    scheduler.CreateNewProcess(processName, memSize);
    proc = scheduler.GetProcess(processName);

    if (proc != nullptr) {
        DisplayProcessScreen(proc); 
    } else {
        cout << "Failed to create process." << endl;
    }
}

void Console::CreateCustomScreen(const string& args) {
    // Parse: name memsize "instruction1; instruction2; ..."
    size_t firstQuote = args.find('"');
    size_t lastQuote = string::npos;
    if (firstQuote != string::npos) lastQuote = args.rfind('"');

    if (firstQuote == string::npos || lastQuote == firstQuote) {
        cout << "Invalid screen -c usage. Format: screen -c <process_name> <process_memory_size> \"<instructions>\"" << endl;
        return;
    }

    string before = args.substr(0, firstQuote);
    string instrStr = args.substr(firstQuote + 1, lastQuote - firstQuote - 1);

    istringstream iss(before);
    string name;
    size_t memSize = 0;

    if (!(iss >> name >> memSize)) {
        cout << "Invalid screen -c usage. Format: screen -c <process_name> <process_memory_size> \"<instructions>\"" << endl;
        return;
    }

    // Validate memory
    if (memSize < 64 || memSize > 65536) {
        cout << "invalid memory allocation" << endl;
        return;
    }

    auto isPow2 = [](size_t x) { return x && !(x & (x - 1)); };
    if (!isPow2(memSize)) {
        cout << "invalid memory allocation" << endl;
        return;
    }

    // Parse semicolon-separated instructions
    vector<string> parts;
    {
        istringstream s(instrStr);
        string token;
        while (getline(s, token, ';')) {
            size_t a = token.find_first_not_of(" \t\r\n");
            if (a == string::npos) continue;
            size_t b = token.find_last_not_of(" \t\r\n");
            parts.push_back(token.substr(a, b - a + 1));
        }
    }

    if (parts.empty() || parts.size() > 50) {
        cout << "invalid command" << endl;
        return;
    }

    // Parse instructions into instruction objects
    vector<Instruction*> instructions;
    for (auto& p : parts) {
        vector<string> toks;

        // Special handling for PRINT
        if (p.rfind("PRINT", 0) == 0) {
            toks.push_back("PRINT");
            size_t open = p.find('(');
            size_t close = p.rfind(')');
            if (open != string::npos && close != string::npos && close > open) {
                string inside = p.substr(open + 1, close - open - 1);
                toks.push_back(inside);
            }
            else {
                toks.push_back("");
            }
        }
        else {
            istringstream iss2(p);
            string tok;
            while (iss2 >> tok) toks.push_back(tok);
        }

        if (toks.empty()) continue;
        string op = toks[0];

        if (op == "DECLARE" && toks.size() >= 3) {
            instructions.push_back(new DeclareInstruction(toks[1], (uint16_t)stoi(toks[2]), nullptr));
        }
        else if (op == "ADD" && toks.size() >= 4) {
            instructions.push_back(new AddInstruction(toks[1], toks[2], (uint16_t)stoi(toks[3]), nullptr));
        }
        else if (op == "SUBTRACT" && toks.size() >= 4) {
            instructions.push_back(new SubtractInstruction(toks[1], toks[2], (uint16_t)stoi(toks[3]), nullptr));
        }
        else if (op == "SLEEP" && toks.size() >= 2) {
            instructions.push_back(new SleepInstruction((uint8_t)stoi(toks[1]), nullptr));
        }
        else if (op == "WRITE" && toks.size() >= 3) {
            uint32_t addr = stoul(toks[1], nullptr, 0);
            uint16_t val = (uint16_t)stoi(toks[2]);
            instructions.push_back(new WriteInstruction(addr, val, nullptr));
        }
        else if (op == "READ" && toks.size() >= 3) {
            uint32_t addr = stoul(toks[2], nullptr, 0);
            instructions.push_back(new ReadInstruction(toks[1], addr, nullptr));
        }
        else if (op == "PRINT") {
            string msg = toks.size() >= 2 ? toks[1] : "";
            instructions.push_back(new PrintInstruction(msg, nullptr));
        }
    }

    if (instructions.empty()) {
        cout << "No valid instructions parsed." << endl;
        return;
    }

    // Create process via scheduler with custom instructions
    // Note: We'll need to add a method in Scheduler for this
    cout << "Creating custom process " << name << " with " << instructions.size() << " instructions." << endl;

    // For now, create a regular process and note limitation
    scheduler.CreateNewProcess(name, memSize);
    Process* proc = scheduler.GetProcess(name);

    if (proc != nullptr) {
        cout << "Note: Custom instruction integration requires scheduler enhancement." << endl;
        DisplayProcessScreen(proc);
    }

    // Clean up instructions
    for (auto instr : instructions) {
        delete instr;
    }
}

/* Reattaches to an existing process screen.
   Connects to Scheduler::GetProcess() to retrieve by name. */
void Console::SearchScreen(const string& processName) {
    Process* proc = scheduler.GetProcess(processName);

    if (proc == nullptr) {
        cout << "Process " << processName << " not found." << endl;
        return;
    }

    // Check for memory violation
    if (proc->HasMemoryViolation()) {
        char timeStr[100];
        tm local_tm;
        time_t vtime = proc->GetViolationTime();
        localtime_s(&local_tm, &vtime);
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &local_tm);

        cout << "Process " << processName << " shut down due to memory access violation error that occurred at "
            << timeStr << ". 0x" << hex << uppercase << proc->GetViolationAddress()
            << " invalid." << dec << endl;
        return;
    }

    DisplayProcessScreen(proc);
}

/* Clears the screen and opens a dedicated interface for one process.
   Handles process-specific commands like “process-smi” and “exit”. */
void Console::DisplayProcessScreen(Process* proc) {
    system("cls"); // comment out to see & verify the instruction types generated

    cout << "Process name: " << proc->GetName() << endl;

    bool inScreen = true;
    while (inScreen) {
        string command;
        cout << "root:\\> ";
        getline(cin, command);

        if (command == "process-smi") {
            // Try to assign the process to an idle core if it currently has -1
            // to fix that -1 huhuhuhu
            if (proc->GetCoreAssigned() == -1) {
                bool assigned = scheduler.TryAssignProcess(proc);
                if (assigned) {
                    cout << "Process " << proc->GetName() << " assigned to core " << proc->GetCoreAssigned() << " immediately." << endl;
                }
            }
            proc->PrintInfo();
        }
        else if (command == "exit") {
            inScreen = false;
            system("cls");
        }
        else {
            cout << "Unknown command in process screen." << endl;
        }
    }
}

/* Lists all processes with their statuses and CPU utilization.
   Connects to Scheduler methods to retrieve process lists and stats. */
void Console::ListScreens() {
    cout << "CPU Utilization: " << fixed << setprecision(2)
        << scheduler.GetCPUUtilization() << "%" << endl;
    cout << "Cores used: " << scheduler.GetCoresUsed() << endl;
    cout << "Cores available: " << scheduler.GetCoresAvailable() << endl;
    cout << "-----------------------------------------------" << endl << endl;

	// Display running processes
    cout << "Running processes:" << endl;
    auto runningProcs = scheduler.GetRunningProcesses();
    if (runningProcs.empty()) {
        cout << "None" << endl;
    }
    else {
        for (auto proc : runningProcs) {
            time_t now = time(nullptr);
            string ts = FormatTimestamp(now);
            string status = proc->IsFinished() ? "Finished" : "Running";
            cout << proc->GetName() << "   "
                << "(" << ts << ")" << "   "
                << status << "   "
                << proc->GetCurrentLine()
                << "/" << proc->GetTotalLines() << endl;
        }
    }

	// Display finished processes
    cout << "\nFinished processes:" << endl;
    auto finishedProcs = scheduler.GetFinishedProcesses();
    if (finishedProcs.empty()) {
        cout << "None" << endl;
    }
    else {
        for (auto proc : finishedProcs) {
            time_t tsTime = proc->GetFinishTime();
            string ts = (tsTime != 0) ? FormatTimestamp(tsTime) : "N/A";
            cout << proc->GetName() << "   "
                << "(" << ts << ")" << "   "
                << "Finished   "
                << proc->GetTotalLines()
                << "/" << proc->GetTotalLines() << endl;
        }
    }
    cout << "-----------------------------------------------" << endl;
}

/* Starts process generation via Scheduler. */
void Console::SchedulerStart() {
    scheduler.Start();
}

/* Stops automatic process generation. */
void Console::SchedulerStop() {
    scheduler.Stop();
}

/* Generates a CPU utilization report into a log file.
   Connects to Scheduler for process data and saves formatted output. */
void Console::ReportUtil() {
    ofstream logFile("csopesy-log.txt");

    if (!logFile.is_open()) {
        cerr << "Error: Could not create log file." << endl;
        return;
    }

    logFile << "CPU Utilization: " << fixed << setprecision(2)
        << scheduler.GetCPUUtilization() << "%" << endl;
    logFile << "Cores used: " << scheduler.GetCoresUsed() << endl;
    logFile << "Cores available: " << scheduler.GetCoresAvailable() << endl;
    logFile << "-----------------------------------------------" << endl << endl;

    logFile << "Running processes:" << endl;
    auto runningProcs = scheduler.GetRunningProcesses();
    if (runningProcs.empty()) {
        logFile << "None" << endl;
    }
    else {
        for (auto proc : runningProcs) {
            time_t now = time(nullptr);
            string ts = FormatTimestamp(now);
            logFile << proc->GetName() << "   "
                << "(" << ts << ")" << "   "
				<< "Core: " << proc->GetCoreAssigned() << "   "
                << proc->GetCurrentLine()
                << "/" << proc->GetTotalLines() << endl;
        }
    }

    logFile << "\nFinished processes:" << endl;
    auto finishedProcs = scheduler.GetFinishedProcesses();
    if (finishedProcs.empty()) {
        logFile << "None" << endl;
    }
    else {
		// Sort finished processes by finish time 
        for (auto proc : finishedProcs) {
            time_t now = time(nullptr);
            string ts = FormatTimestamp(now);
            logFile << proc->GetName() << "   "
                << "(" << ts << ")" << "   "
                << "Finished   "
                << proc->GetTotalLines()
                << "/" << proc->GetTotalLines() << endl;
        }
    }

    logFile << "-----------------------------------------------" << endl;
    logFile.close();

    cout << "Report generated: csopesy-log.txt" << endl;
}

void Console::ProcessSmiGlobal() {
    if (memVis) {
        cout << memVis->ProcessSmi(-1) << endl;
    }
    else {
        cout << "Memory visualizer not initialized." << endl;
    }
}

void Console::VmstatGlobal() {
    if (memVis) {
        cout << memVis->Vmstat() << endl;
    }
    else {
        cout << "Memory visualizer not initialized." << endl;
    }
}