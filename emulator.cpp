/* Entry point + user command recognition and handling */

#include <iostream>
#include <string>
#include <windows.h>
#include <sstream>
#include "console.h"
#include "memoryallocator.h"
#include "memoryvisual.h"

using namespace std;

/* Memory global variables */
MemoryAllocator memAlloc;
MemoryVisual* memVis = nullptr;

void Welcome() {
    cout << " ____  ____  _____  _____  ____  ____  __  __" << endl;
    cout << "|  __|/  __||  _  ||  _  ||  __|/  __||  ||  |" << endl;
    cout << "| |  |  |__ | | | || |_| || |__ | |__ |  \\/  |" << endl;
    cout << "| |   \\_   || | | || ___/ |  __| \\_  | \\_  _/" << endl;
    cout << "| |__  __| || |_| || |    | |__ __|  |  |  |" << endl;
    cout << "|____||____||_____||_|    |____||____|  |__|" << endl;
	cout << "-----------------------------------------------" << endl;
    cout << "Welcome to CSOPESY Emulator!" << endl;
    cout << "\nGroup developer:" << endl;
    cout << "Lim, Nathan\nMagabo, Julianna\nManlapig, Rachel\nSanchez, Jeck" << endl;
    cout << "\nLast Updated: 10-28-2025" << endl;
    cout << "-----------------------------------------------\n" << endl;
}

/* TO BE UPDATED ACCORDINGLY
     console.cpp, 
     instruction.cpp, 
     memoryallocator.cpp, 
     memoryvisual.cpp, 
     process.cpp, 
     scheduler.cpp 
 */

int main() {
    Welcome();
    Console console;
	bool running = true;

    while (running) {
        string command;
        cout << "root:\\> ";
        getline(cin, command);
        
        if (command == "initialize") console.Initialize();
        else if (!console.IsInitialized() && command != "exit") cout << "Please initialize the system first using 'initialize' command." << endl;
        else if (command == "screen -ls") console.ListScreens();
        else if (command.rfind("screen -s ", 0) == 0) console.CreateScreen(command.substr(10));
        else if (command.rfind("screen -r ", 0) == 0) {
            console.SearchScreen(command.substr(10));
            Welcome();
        }
        else if (command.rfind("screen -c", 0) == 0) console.CreateCustomScreen(command.substr(10));
        else if (command == "process-smi") console.ProcessSmiGlobal();
        else if (command == "vmstat") console.VmstatGlobal();
        else if (command == "scheduler-start") console.SchedulerStart();
        else if (command == "scheduler-stop") console.SchedulerStop();
        else if (command == "report-util") console.ReportUtil();
        else if (command == "exit") running = false;
        else cout << "Unknown command. Please try again." << endl;
    }
    // Cleanup
    delete memVis;

    return 0;
}