# Process Multiplexer and Command-line Interpreter

_project description here_

**Group developer:** 
- Lim, Nathan
- Magabo, Julianna
- Manlapig, Rachel
- Sanchez, Jeck

## Requirements

1. Main menu console and `screen` command support

| Command | Description |
|---------|-------------|
| `initialize` | Initialize the processor configuration of the application. This must be called before any other command could be recognized, aside from `exit`. |
| `exit` | Terminates the console. |
| `screen -s <process name>` | Create a new process. |
| `screen -r <process name>` | Access a process. |
| `screen -ls` | Lists all running processes. |
| `scheduler-start` | Continuously generates a batch of dummy processes for the CPU scheduler. Each process is accessible via the `screen` command. |
| `scheduler-stop` | Stops generating dummy processes. |

2. Barebones process instructions

Support basic process instructions, akin to programming language instructions

| Instruction | Description |
|-------------|-------------|
| `PRINT (msg)` | Display an output “msg” to the console. The output can only be seen when the user is inside its attached screen. The “msg” can print 1 variable, “var.” E.g. `PRINT (“Value from: ” +x)` |
| `DECLARE (var, value)` | Declares a uint16 with variable name “var”, and a default “value”. |
| `ADD (var1, var2/value, var3/value)` | Performs an addition operation: `var1 = var2/value + var3/value`. `var1`, `var2`, `var3` are variables. Variables are automatically declared with a value of 0 if they have not yet been declared beforehand. Can also add a uint16 value. |
| `SUBTRACT (var1, var2/value, var3/value)` | Performs a subtraction operation: `var1 = var2/value - var3/value` |
| `SLEEP (X)` | Sleeps the current process for X (uint8) CPU ticks and relinquishes the CPU. |
| `FOR([instructions], repeats)` | Performs a for-loop, given a set/array of instructions. Can be nested. |

3. Generation of CPU utilization report

The console should be able to generate a utilization report whenever the `report-util` command is entered.

4. Configuration setting

The `initialize` commands should read from a `config.txt` file, the parameters for your CPU scheduler and process attributes.

## Implementation

1. Main menu console

**Checklist (to remove)**
[] Main menu console and `screen` command support  

- so far, i implemented `Console.cpp` and `Console.h` to handle the main menu console and the screen command. 
- for example you call the `screen -s <process name>` command, it will create a new process with the given name.
- if you call the `screen -ls` command, it will list all the running processes in a new console window i guess??

[] Process creation and management

`Process.cpp` and `Process.h` to handle the process creation and management.

[] Barebones process instructions  

`Scheduler.cpp` and `Scheduler.h` to handle the CPU scheduling. 

[] Generation of CPU utilization report  

[] Configuration setting

`Config.cpp` and `Config.h` to handle the configuration file reading.

## How to Run

1. Open the folder using Visual Studio
2. Go to File -> Open -> Project/Solution and open the `mco1-cli.sln` file.
3. Run the program!

***If it's not yet a project (no `.sln` file)***

1. File > New > Project From Existing Code..
2. Select Visual C++
3. Browse to the folder of the cloned repository
4. Follow the next steps, then click Finish

***If encountering an "unresolved externals" issue***

1. Right click on the project in the Solution Explorer, then select Properties
2. Linker > System
3. Set Subsystem to `Console (/SUBSYSTEM:CONSOLE)` using the dropdown
4. Apply and rebuild
