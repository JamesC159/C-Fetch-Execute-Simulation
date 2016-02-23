# C-Fetch-Execute-Simulation
This is a C implementation of a CPU doing a fetch/execute cycle.

The CPU is able to fetch instructions from memory to execute. This is done with an implementation of a custom instruction set architecuture

The CPU and Memory are 2 different processes that communicate with each other through 2 different pipes.

Memory is initialized with a program read from a file that is implemented with the custom ISA which can be found in the text files. Each text file implements a different program for demonstration purposes.

User mode and kernel mode is also implemented and considered in this program. User stack and system stack resides within the user and kernel mode spaces. Pushing and poping onto each of these addresses is possible. Pushing onto the kernel mode stack is only possible during interrupts. Trying to push into the kernel mode stack from user mode space will cause a memory error and abort the program.
