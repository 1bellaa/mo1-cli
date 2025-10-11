#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <thread>
#include <atomic>
#include "scheduler.h"

using namespace std;

class Console {
private:
    bool initialized;
    Scheduler scheduler;
    thread* tickThread;
    atomic<bool> shouldRunTicks;

    void TickLoop();
    void DisplayProcessScreen(Process* proc);

public:
    Console();
    ~Console();

    void Initialize();
    bool IsInitialized() const { return initialized; }

    void CreateScreen(const string& processName);
    void SearchScreen(const string& processName);
    void ListScreens();

    void SchedulerStart();
    void SchedulerStop();
    void ReportUtil();
};

#endif