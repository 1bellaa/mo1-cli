#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>
#include <iostream>
#include <map>
#include <set>

using namespace std;

class Console {
    bool initialized = false; // girl idk maybe this is right
    map<string, string> screens; // para magamit sa list screens eme
    set<string> activeScreens; // para malaman yung mga processes i guess?? na hindi pa tapos
    //void ProcessSmi(); // add code later, but this is also a command
    //void ScreenCommands(const string& name); // commands like process-smi, exit

public:
    void Initialize();
    bool IsInitialized();
    void CreateScreen(const string& name);
    void ListScreens();
    void SearchScreen(const string& name);
    void SchedulerStart();
    void SchedulerStop();
    void ReportUtil();
};

#endif