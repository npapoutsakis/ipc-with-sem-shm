
# IPC Process Management with Semaphores and Shared Memory

This project implements a system where a parent process (P) spawns multiple child processes (Ci). The parent process manages communication with the children through semaphores and shared memory. The system simulates a message-passing mechanism, where the parent randomly sends lines of text to the active child processes, and children print the received message. The child processes are terminated based on commands from the parent.

## Features

- Parent process can spawn and terminate child processes.
- Each child waits for a message from the parent process through shared memory.
- Parent sends random messages to active children based on commands from a configuration file.
- Each child prints the received message and terminates when a termination command is issued.
- The use of semaphores ensures synchronized communication between parent and children.
- The project utilizes shared memory for inter-process communication (IPC).

## Prerequisites

- A Unix-based system (Linux, macOS, etc.)
- GCC compiler (or any C compiler that supports POSIX APIs)
- Basic understanding of IPC mechanisms (shared memory, semaphores)
- Configuration file (`config_file.txt`) for defining commands and child behavior

## Files

- **main.c**: Main C file implementing the process management, semaphores, and shared memory handling.
- **config_file.txt**: A text file containing commands for the parent process, defining when to spawn, terminate, or exit child processes.
- **mobydick.txt**: Sample text file used to send random lines to child processes.

## Compilation

To compile the project, use the following command:

```bash
gcc -o ipc_process_manager main.c -lrt
```

- `-lrt`: Links the real-time library (required for IPC-related functionality).

## Usage

### Run the program with the following command:

```bash
./ipc_process_manager <config_file> <text_file> <max_processes>
```

- `<config_file>`: Path to the configuration file (defines the spawn/terminate/exit commands).
- `<text_file>`: Path to the text file from which the parent will send random lines to child processes.
- `<max_processes>`: The maximum number of child processes the parent can manage at once.

### Example:

```bash
./ipc_process_manager config_3_100.txt mobydick.txt 3
```

This will:
- Use the `config_3_100.txt` as the command file.
- Use the `mobydick.txt` file for sending random lines to the child processes.
- Limit the system to a maximum of 3 child processes at a time.

## Configuration File (config_file.txt)

The configuration file should consist of a series of commands that the parent process will follow. Each command follows this format:

```
<timestamp> <child_id> <status>
```

- **timestamp**: The time at which the command should be executed.
- **child_id**: The ID of the child process (e.g., `C0`, `C1`, etc.).
- **status**: The action to be performed (`S` for spawn, `T` for terminate, `EXIT` for termination of the entire program).

### Example of a configuration file:

```
5 C0 S
18 C1 S
25 C0 T
30 C1 T
35 EXIT
```

This file will:
- Spawn child `C0` at timestamp 5.
- Spawn child `C1` at timestamp 18.
- Terminate child `C0` at timestamp 25.
- Terminate child `C1` at timestamp 30.
- Exit the program at timestamp 35.

## How It Works

1. **Spawning Children**: When a spawn command (`S`) is issued, a new child process is created, and the semaphore for that process is initialized. The child waits for a message from the parent via shared memory.

2. **Sending Messages**: The parent randomly selects one of the active child processes and sends a random line from the text file through shared memory. The selected child will print the received message.

3. **Terminating Children**: When a termination command (`T`) is issued, the parent sends a termination signal to the respective child, which will terminate and print information about the number of messages received and the total time it was active.

4. **Cleanup**: When the `EXIT` command is executed, the parent process waits for all child processes to terminate, cleans up shared memory, and removes semaphores.

## Example Output

```
Tick 23:
Parent sent to: C1 the msg: "which had bones of very great value for their teeth, of which he"
Child C1 received msg: "which had bones of very great value for their teeth, of which he"

Tick 24:
Parent sent to: C1 the msg: ""
Child C1 received msg: ""

Tick 25:
Parent sent to: C0 the msg: ""
Termination of C0 with process id 39126

Tick 26:
Parent sent to: C1 the msg: "surf. Right and left, the streets take you waterward. Its extreme"

...
```

## License

This project is open source and available under the MIT License.

## Acknowledgements

- The project was built as part of an assignment for process management and inter-process communication (IPC) in C.
- The use of semaphores and shared memory is based on the POSIX IPC mechanisms.
