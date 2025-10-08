# Process Multiplexer and Command-line Interpreter

_project description here_

**Group developer:** 
- Lim, Nathan
- Magabo, Julianna
- Manlapig, Rachel
- Sanchez, Jeck

## Requirements

1. Main menu console and `Screen` command support

| Command | Description |
|---------|-------------|
| `initialize` | Initialize the processor configuration of the application. This must be called before any other command could be recognized, aside from `exit`. |
| `exit` | Terminates the console. |
| `screen -s <process name>` | Create a new process. |
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

The console should be able to generate a utilization report whenever the “report-util” command is entered.

4. Configuration setting

The `initialize` commands should read from a `config.txt` file, the parameters for your CPU scheduler and process attributes.

## Implementation

1. Main menu console

Implementation

Command Recognition

## How to Run