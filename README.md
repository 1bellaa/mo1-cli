# Multitasking OS

A **Multitasking OS with Memory Management** that simulates a simple command-line interface (CLI) for process management, CPU scheduling, and memory visualization.

**Group developer:** 
- Lim, Nathan
- Magabo, Julianna
- Manlapig, Rachel
- Sanchez, Jeck

## Requirements

1. Main menu console 

Aside from the commands previously implemented in MO1 (`initialize`, `exit`, `screen`, `scheduler-start`, and `scheduler-stop`), additional commands will be included and `screen` commands will be updated:

| Command | Description |
|---------|-------------|
| `process-smi` | Displays a summarized view of the available/used memory, as well as the list of processes and memory occupied. This is similar to the `nvidia-smi` command. |
| `vmstat` | Displays a detailed view of the active/inactive processes, available/used memory, and pages. |
| `screen -c` <process_name> <process_memory_size> "<instructions>" | Sends a string of instructions to be executed by the specified process. Instructions are semicolon-separated. Throws "invalid command" if the instruction size is not met. |
| `screen -r <process name>` | If the process name has prematurely shut down due to a memory access violation error, the console should print "Process <process name> shut down due to memory access violation error that occurred at <HH:MM:SS>. <Hex memory address> invalid." |
| `screen -s <process_name> <process_memory_size>` | Creates a new process with a given name and memory allocation. |

2. Memory Manager

The memory manager should handle the allocation and deallocation of memory for processes. It must support a demand paging allocator. 
This means that the memory manager should be able to allocate memory pages to processes as needed, rather than allocating all memory at once.

3. Memory visualization and backing store access

The program should provide a way to visualize the memory usage of the processes. It will have a way to debug the memory, showing which pages are allocated to which processes, and which pages are free.
The backing store is represented as a text file that can be accessed at any given time. It is saved in a text file `csopesy-backing-store.txt`.

4. Required memory per process

Using the `screen -s` command, the user should be able to specify the amount of memory (in bytes) that a process requires. The memory size must be a power of 2 and within the range of [64, 65536] bytes.

5. Simulating memory access via process instruction

Aside from the basic process instructions implemented in MO1 (`PRINT`, `DECLARE`, `ADD`, `SUBTRACT`, `SLEEP`, `FOR`), the following instructions should be supported:

| Instruction | Description |
|-------------|-------------|
| `READ (var, memory_address)` | Performs a retrieval of a uint16 value from memory and stores it to a variable, var. If the memory block isn’t initialized, the uint16 value is 0. |
| `WRITE (memory_address, value)` | Writes uint16 value to the specified memory address. |

6. User-defined instructions during process creation

Using the `screen -c` command, the user should be able to send a string of 1 – 50 instructions to be executed by the specified process. Instructions are semicolon-separated. Throws “invalid command” if the instruction size is not met.

7. Previous features from MO1

For more information about the previous features in MO1, please refer to the [MO1 README](https://github.com/1bellaa/mo1-cli/tree/mo1) file.

## Implementation

_to add later_

## How to Run

1. Open the folder using Visual Studio
2. Go to **File -> Open -> Project/Solution** and open the `mco1-cli.sln` file.
3. Run the program!

***If it's not yet a project (or no `.sln` file)***

1. **File > New > Project From Existing Code..**
2. Select **Visual C++**
3. Browse to the folder of the cloned repository
4. Follow the next steps, then click Finish

***If encountering an "unresolved externals" issue***

1. Right click on the project in the Solution Explorer, then select Properties
2. Linker > System
3. Set Subsystem to `Console (/SUBSYSTEM:CONSOLE)` using the dropdown
4. Apply and rebuild
