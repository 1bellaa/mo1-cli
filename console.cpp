/* CLI interface + I/O commands with memory management features */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif
#include "console.h"
#include "scheduler.h"

using namespace std;

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
   Connects to Scheduler::Initialize() to load config.txt.
   Now also initializes memory manager with configured parameters. */
void Console::Initialize() {
    scheduler.Initialize("config.txt");
    initialized = true;

    shouldRunTicks = true;
    tickThread = new thread(&Console::TickLoop, this);

    cout << "System initialized successfully." << endl;
}

/* Simulates the CPU ticking mechanism in real-time.
   Continuously calls Scheduler::Tick() to update process states. */
void Console::TickLoop() {
    while (shouldRunTicks) {
        scheduler.Tick();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

/* Utility to format timestamps consistently.
   Uses platform-specific localtime functions for cross-compatibility. */
static string FormatTimestamp(time_t t) {
    char buf[64];
    tm local_tm;
#ifdef _WIN32
    localtime_s(&local_tm, &t);
#else
    localtime_r(&t, &local_tm);
#endif
    strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S %p", &local_tm);
    return string(buf);
}

/* Creates a new process "screen" for user interaction with specified memory size.
   Connects to Scheduler::CreateNewProcess() to spawn a process.
   Validates that memory size is between 64 and 65536 bytes and is a power of 2. */
void Console::CreateScreen(const string& processName, int memorySize) {
    // Validate memory size
    if (memorySize < 64 || memorySize > 65536) {
        cout << "Invalid memory allocation. Memory must be between 64 and 65536 bytes." << endl;
        return;
    }

    // Check if power of 2
    if ((memorySize & (memorySize - 1)) != 0) {
        cout << "Invalid memory allocation. Memory must be a power of 2." << endl;
        return;
    }

    Process* proc = scheduler.GetProcess(processName);
    if (proc != nullptr) {
        cout << "Process " << processName << " already exists." << endl;
        return;
    }

    scheduler.CreateNewProcess(processName, memorySize);
    proc = scheduler.GetProcess(processName);

    if (proc != nullptr) {
        DisplayProcessScreen(proc);
    }
}

/* Creates a new process with user-defined instructions and specified memory size.
   Validates memory size (64-65536 bytes, power of 2) and instruction count (1-50). */
void Console::CreateScreenWithInstructions(const string& processName, int memorySize, const string& instructions) {
    // Validate memory size
    if (memorySize < 64 || memorySize > 65536) {
        cout << "Invalid memory allocation. Memory must be between 64 and 65536 bytes." << endl;
        return;
    }

    if ((memorySize & (memorySize - 1)) != 0) {
        cout << "Invalid memory allocation. Memory must be a power of 2." << endl;
        return;
    }

    // Parse and validate instructions 
    // Count semicolons to estimate instruction count
    int instrCount = 1;
    for (char c : instructions) {
        if (c == ';') instrCount++;
    }

    if (instrCount < 1 || instrCount > 50) {
        cout << "Invalid command. Instruction count must be between 1 and 50." << endl;
        return;
    }

    // Create a basic process
    cout << "User-defined instruction processes not fully implemented yet." << endl;
    cout << "Creating process with random instructions instead." << endl;
    CreateScreen(processName, memorySize);
}

/* Reattaches to an existing process screen.
   Connects to Scheduler::GetProcess() to retrieve by name.
   Now checks for memory access violations and displays error message if applicable. */
void Console::SearchScreen(const string& processName) {
    Process* proc = scheduler.GetProcess(processName);

    if (proc == nullptr) {
        cout << "Process " << processName << " not found." << endl;
        return;
    }

    if (proc->HasMemoryError()) {
        time_t errTime = proc->GetMemoryErrorTime();
        tm local_tm;
#ifdef _WIN32
        localtime_s(&local_tm, &errTime);
#else
        localtime_r(&errTime, &local_tm);
#endif
        char buf[64];
        strftime(buf, sizeof(buf), "%H:%M:%S", &local_tm);

        cout << "Process " << processName << " shut down due to memory access violation error that occurred at "
            << buf << ". 0x" << hex << proc->GetMemoryErrorAddress() << dec << " invalid." << endl;
        return;
    }

    DisplayProcessScreen(proc);
}

/* Clears the screen and opens a dedicated interface for one process.
   Handles process-specific commands like "process-smi" and "exit". */
void Console::DisplayProcessScreen(Process* proc) {
    system(CLEAR_SCREEN);

    cout << "Process name: " << proc->GetName() << endl;

    bool inScreen = true;
    while (inScreen) {
        string command;
        cout << "root:\\> ";
        getline(cin, command);

        if (command == "process-smi") {
            if (proc->GetCoreAssigned() == -1 && !proc->IsFinished()) {
                bool assigned = scheduler.TryAssignProcess(proc);
                if (assigned) {
                    cout << "Process " << proc->GetName() << " assigned to core " << proc->GetCoreAssigned() << " immediately." << endl;
                }
            }
            proc->PrintInfo();
        }
        else if (command == "exit") {
            inScreen = false;
            system(CLEAR_SCREEN);
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

    cout << "Running processes:" << endl;
    auto runningProcs = scheduler.GetRunningProcesses();
    if (runningProcs.empty()) {
        cout << "None" << endl;
    }
    else {
        for (auto proc : runningProcs) {
            time_t now = time(nullptr);
            string ts = FormatTimestamp(now);

            string status;
            int coreId = proc->GetCoreAssigned();

            if (coreId >= 0) {
                status = "Core: " + to_string(coreId);
            }
            else {
                status = "Ready";
            }

            cout << proc->GetName() << "   "
                << "(" << ts << ")" << "   "
                << status << "   "
                << proc->GetCurrentLine()
                << "/" << proc->GetTotalLines() << endl;
        }
    }

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

            string status;
            int coreId = proc->GetCoreAssigned();

            if (coreId >= 0) {
                status = "Core: " + to_string(coreId);
            }
            else {
                status = "Ready";
            }

            logFile << proc->GetName() << "   "
                << "(" << ts << ")" << "   "
                << status << "   "
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
        for (auto proc : finishedProcs) {
            time_t tsTime = proc->GetFinishTime();
            string ts = (tsTime != 0) ? FormatTimestamp(tsTime) : "N/A";
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

/* Displays system memory information and running process memory usage.
   New command for MO2: Shows CPU and memory utilization in KiB units.
   Connects to MemoryManager for memory statistics. */
void Console::ProcessSMI() {
    MemoryManager* memMgr = scheduler.GetMemoryManager();

    // Get memory values
    int usedMem = memMgr->GetUsedMemory();
    int totalMem = memMgr->GetTotalMemory();

    // Calculate percentages
    double usedPercent = (totalMem > 0) ? (usedMem * 100.0 / totalMem) : 0.0;
    double cpuUtil = scheduler.GetCPUUtilization();

    cout << "=============================================" << endl;
    cout << "| PROCESS-SMI V01.00 Driver Version: 01.00 |" << endl;
    cout << "=============================================" << endl;
    cout << "CPU-Util: " << fixed << setprecision(0) << cpuUtil << "%" << endl;
    cout << "Memory Usage: " << usedMem << "MiB / " << totalMem << "MiB" << endl;
    cout << "Memory Util: " << fixed << setprecision(0) << usedPercent << "%" << endl;
    cout << "=============================================" << endl;
    cout << endl;
    cout << "Running processes and memory usage:" << endl;
    cout << "--------------------------------------------" << endl;

    auto runningProcs = scheduler.GetRunningProcesses();
    if (runningProcs.empty()) {
        cout << "No running processes." << endl;
    }
    else {
        for (auto proc : runningProcs) {
            // Convert bytes to MiB for display
            int memMiB = proc->GetMemorySize() / (1024 * 1024);
            if (memMiB == 0) memMiB = 1; // Show at least 1MiB for small allocations

            cout << proc->GetName() << " " << memMiB << "MiB" << endl;
        }
    }

    cout << "--------------------------------------------" << endl;
}

/* Displays virtual memory statistics including paging information.
   New command for MO2: Shows total/used/free memory, CPU tick statistics,
   and page-in/page-out counts. Saves backing store to file.
   Connects to MemoryManager and Scheduler for statistics. */
void Console::VMStat() {
    MemoryManager* memMgr = scheduler.GetMemoryManager();

    cout << "\n=== VM Statistics ===" << endl;
    cout << "-----------------------------------------------" << endl;
    cout << "Total Memory:      " << memMgr->GetTotalMemory() << " bytes" << endl;
    cout << "Used Memory:       " << memMgr->GetUsedMemory() << " bytes" << endl;
    cout << "Free Memory:       " << memMgr->GetFreeMemory() << " bytes" << endl;
    cout << "-----------------------------------------------" << endl;
    cout << "Idle CPU Ticks:    " << scheduler.GetIdleCpuTicks() << endl;
    cout << "Active CPU Ticks:  " << scheduler.GetActiveCpuTicks() << endl;
    cout << "Total CPU Ticks:   " << scheduler.GetCPUTicks() << endl;
    cout << "-----------------------------------------------" << endl;
    cout << "Num Paged In:      " << memMgr->GetNumPagedIn() << endl;
    cout << "Num Paged Out:     " << memMgr->GetNumPagedOut() << endl;
    cout << "===============================================" << endl;

    // Save backing store
    memMgr->SaveBackingStore();

}
