# C-Fetch-Execute-Simulation
This is a C implementation of a CPU doing a fetch/execute cycle.

The CPU is able to fetch instructions from memory to execute. This is done with an implementation of a custom instruction set architecuture

The CPU and Memory are 2 different processes that communicate with each other through a 2 different pipes. There is no storing in this ISA.

Memory is initialized with a program read from a file that is implemented with the custom ISA which can be found in the text files. Each text file implements a different program for demonstration purposes.
