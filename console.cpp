/* CLI interface + I/O commands */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <windows.h>
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
   Connects to Scheduler::Initialize() to load config.txt. */
void Console::Initialize() {
    scheduler.Initialize("config.txt");
    initialized = true;

    shouldRunTicks = true;
    tickThread = new thread(&Console::TickLoop, this);

    //cout << "Console initialized successfully." << endl;
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
    if (proc != nullptr) {
        cout << "Process " << processName << " already exists." << endl;
        return;
    }

    scheduler.CreateNewProcess(processName);
    proc = scheduler.GetProcess(processName);

    if (proc != nullptr) {
        DisplayProcessScreen(proc); 
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