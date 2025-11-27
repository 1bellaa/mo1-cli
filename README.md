# Multitasking OS

A **Multitasking OS with Memory Management** that simulates a simple command-line interface (CLI) for process management, CPU scheduling, demand paging, and memory visualization.

**Group developer:** 
- Lim, Nathan
- Magabo, Julianna
- Manlapig, Rachel
- Sanchez, Jeck

**Last Updated:** 11-27-2025

---

## Features Overview

This project implements a multitasking operating system emulator with the following key features:

### MO1 Features (Process Scheduling)
- **Process Management**: Create, list, and manage processes
- **CPU Scheduling**: Support for FCFS and Round Robin algorithms
- **Process Instructions**: PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR loop
- **Multi-core Simulation**: Configurable number of CPU cores
- **Real-time Scheduling**: Background tick thread for continuous process execution

### MO2 Features (Memory Management)
- **Demand Paging**: Pages loaded into physical memory only when needed
- **Page Fault Handling**: Automatic page replacement when memory is full
- **Memory Visualization**: `process-smi` and `vmstat` commands for debugging
- **Backing Store**: Text file persistence for paged-out memory
- **Memory Access Instructions**: READ and WRITE operations with validation
- **Memory Access Violations**: Proper error handling and process termination
- **User-defined Instructions**: Create processes with custom instruction sets

---

## Commands

### Main Menu Console

| Command | Description |
|---------|-------------|
| `initialize` | Loads configuration from `config.txt` and starts the system |
| `screen -ls` | Lists all running and finished processes with their status |
| `screen -s <name> <memory_size>` | Creates a new process with specified name and memory allocation (64-65536 bytes, power of 2) |
| `screen -c <name> <memory_size> "<instructions>"` | Creates a process with user-defined semicolon-separated instructions (1-50 instructions) |
| `screen -r <name>` | Reattaches to an existing process screen or shows memory error details |
| `scheduler-test` | Starts automatic process generation (alias for `scheduler-start`) |
| `scheduler-start` | Begins automatic process generation at configured frequency |
| `scheduler-stop` | Stops automatic process generation |
| `report-util` | Generates `csopesy-log.txt` with CPU and process statistics |
| `process-smi` | Displays memory usage summary (similar to `nvidia-smi`) |
| `vmstat` | Shows detailed memory and CPU statistics, saves backing store |
| `exit` | Exits the emulator |

### Process Screen Commands

When inside a process screen (after `screen -r <name>`):

| Command | Description |
|---------|-------------|
| `process-smi` | Displays process details including logs and execution progress |
| `exit` | Returns to main menu |

---

## Process Instructions

### Basic Instructions (MO1)

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| `PRINT` | `PRINT "message"` | Logs a timestamped message with CPU core ID |
| `DECLARE` | `DECLARE var value` | Creates a uint16 variable (max 32 variables) |
| `ADD` | `ADD result operand1 operand2` | Adds operand1 + operand2, stores in result |
| `SUBTRACT` | `SUBTRACT result operand1 operand2` | Subtracts operand1 - operand2, stores in result |
| `SLEEP` | `SLEEP cycles` | Pauses process for specified CPU cycles |
| `FOR` | `FOR iterations { ... }` | Repeats nested instructions N times |

### Memory Instructions (MO2)

| Instruction | Syntax | Description |
|-------------|--------|-------------|
| `READ` | `READ var address` | Reads uint16 value from memory address into variable |
| `WRITE` | `WRITE address value` | Writes uint16 value to memory address |

**Example:**
```
screen -c process1 256 "DECLARE varA 10; DECLARE varB 5; ADD varA varA varB; WRITE 0x500 varA; READ varC 0x500; PRINT Result"
```

---

## Implementation Details

### 1. Memory Manager

The memory manager implements **demand paging** with the following features:

- **Page Allocation**: Pages are loaded into physical memory frames only when accessed
- **Page Fault Handling**: When a page is not in memory, the system:
  1. Finds a victim frame (using round-robin selection)
  2. Pages out the victim to backing store if necessary
  3. Pages in the requested page
  4. Restarts the instruction
- **Backing Store**: Paged-out data stored in `csopesy-backing-store.txt`
- **Memory Validation**: Process memory must be power of 2, range [64, 65536] bytes
- **Frame Management**: Physical memory divided into fixed-size frames (configurable)

#### Key Components:
- `MemoryManager::AllocateMemory()` - Creates page table for process (demand paging)
- `MemoryManager::DeallocateMemory()` - Frees frames and backing store entries
- `MemoryManager::AccessMemory()` - Handles page faults and frame allocation
- `MemoryManager::ReadMemory()` / `WriteMemory()` - Process memory operations

### 2. Process Representation

Each process contains:
- **Process ID (PID)**: Unique identifier
- **State**: READY, RUNNING, WAITING, FINISHED
- **Memory Size**: Allocated memory in bytes (power of 2)
- **Page Table**: Maps virtual pages to physical frames
- **Symbol Table**: Stores up to 32 uint16 variables (64 bytes)
- **Instruction List**: Generated or user-defined instructions
- **Output Log**: Timestamped execution logs

#### Memory Layout:
```
Symbol Table (64 bytes, fixed)
??? Variable storage (max 32 × 2 bytes)
??? Managed automatically by DECLARE

Process Memory (64-65536 bytes, configurable)
??? Pages allocated on-demand
??? READ/WRITE operations
??? Page fault handling
```

### 3. Scheduler Implementation

**FCFS (First-Come, First-Served)**:
- Non-preemptive scheduling
- Processes execute until completion or WAITING state
- FIFO queue for ready processes

**Round Robin (RR)**:
- Preemptive scheduling with quantum cycles
- Processes execute for quantum, then preempted to ready queue
- Quantum counter per process

**Memory Integration**:
- Instructions execute only when required pages are in memory
- Page faults trigger before instruction execution
- Memory allocated via `MemoryManager` on process creation
- Memory deallocated when process finishes or has memory error

### 4. Demand Paging and Backing Store

**Demand Paging Flow**:
```
1. Process references memory address
2. Check if page in physical memory (page table)
3. If not present (page fault):
   a. Find victim frame (round-robin)
   b. Page out victim if frame occupied
   c. Page in requested page
   d. Update page table
4. Execute instruction
```

**Backing Store Operations**:
- Text-based storage in `csopesy-backing-store.txt`
- Format: `P<pid>_Page<num>` ? `PageData_...`
- Persistent across page-in/page-out operations
- Saved automatically by `vmstat` command

### 5. Memory Access Violations

When a process attempts to access invalid memory:
1. `ReadMemoryAddress()` / `WriteMemoryAddress()` throws `runtime_error`
2. `Process::Execute()` catches exception
3. Process marked with memory error flag
4. Error time and invalid address stored
5. Process state set to FINISHED
6. `screen -r` displays error message with timestamp and address

---

## Configuration File

The `config.txt` file uses space-separated key-value pairs:

### CPU Scheduling Parameters
```
num-cpu 4                    # Number of CPU cores [1, 128]
scheduler "rr"               # Algorithm: "fcfs" or "rr"
quantum-cycles 5             # Time slice for Round Robin [1, 2^32]
batch-process-freq 1         # Process generation frequency [1, 2^32]
min-ins 1000                 # Min instructions per process [1, 2^32]
max-ins 2000                 # Max instructions per process [1, 2^32]
delays-per-exec 0            # Delay between instructions [0, 2^32]
```

### Memory Management Parameters
```
max-overall-mem 16384        # Total memory in bytes [2^6, 2^16]
mem-per-frame 256            # Frame/page size in bytes [2^6, 2^16]
min-mem-per-proc 256         # Min memory per process [2^6, 2^16]
max-mem-per-proc 1024        # Max memory per process [2^6, 2^16]
```

---

## Error Handling

### Memory Validation Errors
- **Invalid memory size**: "Invalid memory allocation. Memory must be between 64 and 65536 bytes."
- **Not power of 2**: "Invalid memory allocation. Memory must be a power of 2."
- **Process exists**: "Process <name> already exists."

### Memory Access Violations
When `screen -r` is used on a crashed process:
```
Process process1 shut down due to memory access violation error that occurred at 14:32:15. 0x2000 invalid.
```

### Instruction Validation
- **Invalid instruction count**: "Invalid command. Instruction count must be between 1 and 50."
- **Process not found**: "Process <name> not found."

---

## How to Run

### Using Visual Studio

1. Open the folder using Visual Studio
2. Go to **File ? Open ? Project/Solution** and open the `mo2.sln` file
3. Run the program (F5 or Ctrl+F5)

### Creating a New Project from Source

**If no `.sln` file exists:**

1. **File ? New ? Project From Existing Code...**
2. Select **Visual C++**
3. Browse to the folder of the cloned repository
4. Follow the next steps, then click **Finish**

### Troubleshooting

**If encountering "unresolved externals" error:**

1. Right-click on the project in the Solution Explorer, then select **Properties**
2. Navigate to **Linker ? System**
3. Set **Subsystem** to `Console (/SUBSYSTEM:CONSOLE)` using the dropdown
4. Click **Apply** and rebuild the project
