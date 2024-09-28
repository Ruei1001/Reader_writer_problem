Shared Memory Communication with Semaphores
Description
This project demonstrates inter-process communication (IPC) using shared memory and semaphores in a Linux environment. It creates a simple message-passing system where one process writes data to shared memory, and two child processes read from it. The synchronization between the processes is handled using semaphores to prevent race conditions.

Key Components
Shared Memory: Used to store a message that the writer process writes and the reader processes read.

Semaphores: Used for synchronizing the access to the shared memory and signaling between the writer and reader processes.

SEM_KEY (0x1234): Handles the synchronization between the writer and readers.
SEM2_KEY (0x9487): Ensures proper locking and unlocking of the shared memory between the readers.
Files and Structure
shared_memory struct: Holds a char array of size 1024 for the message exchange.
writer function: Allows the parent process to write a message into shared memory and signals the readers.
reader function: Two child processes that read the message from shared memory after receiving a signal from the writer.
Workflow
Initialization:

Shared memory is created using shmget and mapped using shmat.
Two semaphores are created for synchronization:
One semaphore controls the write access and the signaling between processes.
The second semaphore ensures that the shared memory is locked during access.
Process Forking:

Two child processes are spawned using fork(). These will act as the readers.
The parent process will be responsible for writing data into the shared memory.
Communication:

The writer waits for permission to write, then locks the shared memory, writes the message, and unlocks it.
After writing, it signals both child processes (readers) to read the message.
Each reader process waits for the signal, reads the message, and unlocks the memory for the next operation.
Termination:

The program terminates once the parent process enters a "-" as the message, which signals the end of the communication.
Usage Instructions
Compile the program using:

gcc -o ipc_program ipc_program.c
Run the program:


./ipc_program
The parent process (writer) will prompt you to enter messages. The two child processes will display the messages read from the shared memory.

To terminate the program, enter - as the message in the parent process.

Important Notes
The parent process waits for both child processes to read the message before allowing the next message to be written.
The semaphores are used to ensure that no process accesses the shared memory simultaneously, preventing race conditions.
Ensure that semaphores and shared memory are cleaned up after use to avoid system resource leaks.
Dependencies
Requires the sys/ipc.h, sys/shm.h, sys/sem.h, and sys/mman.h libraries for shared memory and semaphore operations.
License
This project is released under the MIT License.
