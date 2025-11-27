#ifndef CONSOLE_H
#define CONSOLE_H

#include <thread>
#include "scheduler.h"

using namespace std;

class Console {
private:
    Scheduler scheduler;
    bool initialized;
    thread* tickThread;
    bool shouldRunTicks;

    void TickLoop();
    void DisplayProcessScreen(Process* proc);

public:
    Console();
    ~Console();

    void Initialize();
    bool IsInitialized() const { return initialized; }

    void CreateScreen(const string& processName, int memorySize);
    void CreateScreenWithInstructions(const string& processName, int memorySize, const string& instructions);
    void SearchScreen(const string& processName);
    void ListScreens();

    void SchedulerStart();
    void SchedulerStop();
    void ReportUtil();

    // New MO2 commands
    void ProcessSMI();
    void VMStat();
};

#endif