/* Entry point + user command recognition and handling with memory management */

#include <iostream>
#include <string>
#include <sstream>
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
    cout << "\nLast Updated: 11-27-2025" << endl;
    cout << "-----------------------------------------------\n" << endl;
}

/* Parse screen -s command to extract process name and memory size.
   Returns true if parsing successful, false otherwise.
   Used for manual process creation with specified memory allocation. */
bool ParseScreenCommand(const string& command, string& processName, int& memorySize) {
    istringstream iss(command);
    string cmd, flag, name, memStr;

    iss >> cmd >> flag >> name >> memStr;

    if (cmd != "screen" || name.empty()) {
        return false;
    }

    processName = name;

    if (!memStr.empty()) {
        try {
            memorySize = stoi(memStr);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    return false;
}

/* Parse screen -c command for user-defined instruction processes.
   Extracts process name, memory size, and instruction string from command.
   Returns true if parsing successful, false otherwise.
   Instructions should be enclosed in quotes. */
bool ParseScreenCCommand(const string& command, string& processName, int& memorySize, string& instructions) {
    size_t pos = command.find("screen -c ");
    if (pos == string::npos) {
        return false;
    }

    string remainder = command.substr(10); // Skip "screen -c "
    istringstream iss(remainder);

    iss >> processName >> memorySize;

    // Extract instructions between quotes
    size_t quoteStart = remainder.find('"');
    size_t quoteEnd = remainder.rfind('"');

    if (quoteStart != string::npos && quoteEnd != string::npos && quoteEnd > quoteStart) {
        instructions = remainder.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        return true;
    }

    return false;
}

/* Main entry point: handles command loop and user input.
   Recognizes commands: initialize, screen -ls, screen -s, screen -c, screen -r,
   scheduler-test, scheduler-start, scheduler-stop, report-util, process-smi, vmstat, exit.
   Now includes memory management commands (process-smi, vmstat) for MO2. */
int main() {
    Welcome();
    Console console;
    bool running = true;

    while (running) {
        string command;
        cout << "root:\\> ";
        getline(cin, command);

        if (command == "initialize") {
            console.Initialize();
        }
        else if (!console.IsInitialized() && command != "exit") {
            cout << "Please initialize the system first using 'initialize' command." << endl;
        }
        else if (command == "screen -ls") {
            console.ListScreens();
        }
        else if (command.rfind("screen -s ", 0) == 0) {
            string processName;
            int memorySize;

            if (ParseScreenCommand(command, processName, memorySize)) {
                console.CreateScreen(processName, memorySize);
                Welcome();
            }
            else {
                cout << "Invalid command format. Usage: screen -s <process_name> <memory_size>" << endl;
            }
        }
        else if (command.rfind("screen -c ", 0) == 0) {
            string processName, instructions;
            int memorySize;

            if (ParseScreenCCommand(command, processName, memorySize, instructions)) {
                console.CreateScreenWithInstructions(processName, memorySize, instructions);
                Welcome();
            }
            else {
                cout << "Invalid command format. Usage: screen -c <process_name> <memory_size> \"<instructions>\"" << endl;
            }
        }
        else if (command.rfind("screen -r ", 0) == 0) {
            console.SearchScreen(command.substr(10));
            Welcome();
        }
        else if (command == "scheduler-test" || command == "scheduler-start") {
            console.SchedulerStart();
        }
        else if (command == "scheduler-stop") {
            console.SchedulerStop();
        }
        else if (command == "report-util") {
            console.ReportUtil();
        }
        else if (command == "process-smi") {
            console.ProcessSMI();
        }
        else if (command == "vmstat") {
            console.VMStat();
        }
        else if (command == "exit") {
            running = false;
        }
        else if (command.empty()) {
            // Do nothing for empty input
        }
        else {
            cout << "Unknown command. Please try again." << endl;
        }
    }
    return 0;
}