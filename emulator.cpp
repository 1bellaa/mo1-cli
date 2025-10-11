#include <iostream>
#include <string>
#include <windows.h>
#include "console.h"

using namespace std;

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
    cout << "\nLast Updated: 10-11-2025" << endl;
    cout << "-----------------------------------------------" << endl;
}

int main() {
    Welcome();
    Console console;
	bool running = true;

    while (running) {
        string command;
        cout << "root:\\> ";
        getline(cin, command);
        
        if (command == "initialize") console.Initialize();
        
        /*config file is read in Console class's initialize --> the Scheduler class's loadConfig method*/
        else if (!console.IsInitialized() && command != "exit") cout << "Please initialize the system first using 'initialize' command." << endl;
        
        else if (command == "screen -ls") console.ListScreens();
        else if (command.rfind("screen -s ", 0) == 0) console.CreateScreen(command.substr(10));
        else if (command.rfind("screen -r ", 0) == 0) console.SearchScreen(command.substr(10));
        else if (command == "scheduler-start") console.SchedulerStart();
        else if (command == "scheduler-stop") console.SchedulerStop();
        else if (command == "report-util") console.ReportUtil();
        else if (command == "exit") running = false;
        else cout << "Unknown command. Please try again." << endl;
    }
    return 0;
}