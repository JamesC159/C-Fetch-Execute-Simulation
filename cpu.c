#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>

int readVal(int idx, int pipe1[], int pipe2[])
{
	int store = 0;
	int val = 0;

	write(pipe1[1], &store, sizeof(int));
	write(pipe1[1], &idx, sizeof(int)); // Fetch the value from memory
	read(pipe2[0], &val, sizeof(int)); // Read the value written back to the CPU
	return val;
}

void writeVal(int idx, int val, int pipe1[])
{
	int store = 1;
	
	write(pipe1[1], &store, sizeof(int));
	write(pipe1[1], &idx, sizeof(int));
	write(pipe1[1], &val, sizeof(int));
}

void checkValidAddress(int isUserMode, int idx, int childPid)
{
	if(isUserMode && idx >= 1000)
	{
		fprintf(stderr, "Memory Violation: accessing system address %d in user mode\n", idx);
		kill(SIGTERM, childPid);
		exit(1);
	}
}

int main (int arc, char **argv)
{

	int pipe1[2];		// File descriptors for PC writing to memory
	int pipe2[2];		// File descriptors for PC Reading from memory
	pid_t childPid;		// Child Process ID
	int status;		// Status returned from waiting for the child to finish

	if (((pipe(pipe1)) < 0 )|| (pipe(pipe2)) < 0 )
	{
		perror("pipe");
		exit(1);
	}

	if ((childPid = fork()) == -1)
	{
		perror("fork");
		exit(1);
	}

	if (childPid == -1)
	{
		perror("childPid");
		exit(1);
	}
	else if (childPid == 0)	//Child = Memory
	{
		int mem[2000];	// Memory that holds 2000 integers that represent instructions
			
		// File names to use as tests
		const char *sample1 = "sample3.txt";	// Constant string for file to open
		
		FILE *file;	// File to be read into memory array
		char *str = malloc(sizeof(char) * 50);	// Contains strings from the file
		int numRead = 100;	// Number of bytes to read from the file
		int num = 0;		// Actual instruction to reside in memory
		int idx = 0;		// Index into memory
		int isWriting = 0;
		int val = 0;		

		// Check to see if memory can open the sample file
		if ((file = fopen(sample1, "r")) == NULL)
		{
			fprintf(stderr, "Could no successfully open file %s, exiting now...\n", sample1);
			_exit(1);
		}

		// Loop through file, grabbing each line until the end of the file.
		while (fgets(str, numRead, file) != NULL)
		{
			// Check to see if EOF was read while trying to read a real string
			if (feof(file))
			{
				fprintf(stderr, "Error occuring while trying to read a string\n");
				break;
			}
				
			// Check to see if an error happened while reading
			if (ferror(file))
			{
				fprintf(stderr, "Error reading from the file\n");
				_exit(1);
			}

			// Fill memory with instrutions
			char *token;
			char *delims = " ";
			token = strtok(str, delims);	// We really dont care about destroyign the rest of the string

			if (token[0] == '.')
			{
				token++;
				if (isdigit(token[0]))
				{
					idx = atoi(token);
				}
			}
			else if (isdigit(token[0]))
			{
				num = atoi(token);
				mem[idx++] = num;
			}
			else
			{
				continue;
			}
		}

		//Free allocated memory 
		free (str);

		// Close files
		fclose(file);

		// Create Pipes for Mem to CPU communication
		close(pipe2[0]);
		close(pipe1[1]);
		dup2(pipe2[1], STDOUT_FILENO);
		dup2(pipe1[0], STDIN_FILENO);

		while(1)
		{
			read(pipe1[0], &isWriting, sizeof(int));
			
			if (!isWriting)
			{
				read(pipe1[0], &idx, sizeof(int));
			
				// Write instruction from memory to pipe for CPU to read
				write(pipe2[1], &(mem[idx]), sizeof(int));
			}
			else
			{
				read(pipe1[0], &idx, sizeof(int));
				read(pipe1[0], &val, sizeof(int));
				mem[idx] = val;
			}

		}

		_exit(0);
	}
	else 	//Parent = CPU
	{
		//CPU Registers
		int PC = 0;
		int SP = 1000;
		int IR = 0;
		int AC = 0;
		int X = 0;
		int Y = 0;
		int store = 0;

		int pidReturned = 0;
		int shouldExit = 0;	// Used to signal break from main execution loop
		int isUserMode = 1;

		// Create pipes for CPU to Mem communication
		close(pipe1[0]);
		close(pipe2[1]);
		dup2(pipe1[1], STDOUT_FILENO);
		dup2(pipe2[0], STDIN_FILENO);

		while (!shouldExit)
		{
			IR = readVal(PC, pipe1, pipe2);	// Fetch instruction

			// Switch block to determine which instruction needs to execute
			switch (IR)
			{
				case 1:	// Load value
				{
					PC++;
					AC = readVal(PC, pipe1, pipe2); // Load value into AC
					break;
				}
				case 2: // Load addr
				{
					PC++;
					AC = readVal(PC, pipe1, pipe2);
					checkValidAddress(isUserMode, AC, childPid);
					AC = readVal(AC, pipe1, pipe2);
					break;
				}
				case 3:	// LoadInd addr
				{
					PC++;
					AC = readVal(PC, pipe1, pipe2);
					checkValidAddress(isUserMode, AC, childPid);
					AC = readVal(AC, pipe1, pipe2);
					checkValidAddress(isUserMode, AC, childPid);
					AC = readVal(AC, pipe1, pipe2);	
					break;
				}
				case 4:	// LoadIdxX addr
				{
					PC++;
					AC = readVal(PC, pipe1, pipe2);
					AC += X;
					checkValidAddress(isUserMode, AC, childPid);
					AC = readVal(AC, pipe1, pipe2);
					break;
				}
				case 5:
				{
					PC++;
					AC = readVal(PC, pipe1, pipe2);
					AC += Y;
					checkValidAddress(isUserMode, AC, childPid);
					AC = readVal(AC, pipe1, pipe2);
					break;
				}
				case 6:
				{
					int SPX = SP + X + 1;
					checkValidAddress(isUserMode, SPX, childPid);
					AC = readVal(SPX, pipe1, pipe2);
					break;
				}
				case 7:
				{
					PC++;
					int idx;
					idx = readVal(PC, pipe1, pipe2);
					checkValidAddress(isUserMode, idx, childPid);
					writeVal(idx, AC, pipe1);
					break;
				}
				case 8:
				{
					srand(time(NULL));
					AC = rand() % 100;
					break;
				}
				case 9: // Put port
				{
					PC++;
					int port;
					port = readVal(PC, pipe1, pipe2);
					
					if (port == 1)
					{
						fprintf(stderr, "%d", AC);
					}
					else if (port == 2)
					{
						char c = 0x0 | AC;
						fprintf(stderr, "%c", c);

					}
					else
					{
						fprintf(stderr, "Bad port number: %d\n", AC);
						exit(1);
					}
					
					break;
				}
				case 10:
				{
					AC += X;
					break;
				}
				case 11:
				{
					AC += Y;
					break;
				}
				case 12:
				{
					AC -= X;
					break;
				}
				case 13:
				{
					AC -= Y;
					break;
				}
				case 14:
				{
					X = AC;
					break;
				}
				case 15:
				{
					AC = X;
					break;
				}
				case 16:
				{
					Y = AC;
					break;
				}
				case 17:
				{
					AC = Y;
					break;
				}
				case 18:
				{
					SP = AC;
					break;
				}
				case 19:
				{
					AC = SP;
					break;
				}
				case 20:
				{
					PC++;
					PC = readVal(PC, pipe1, pipe2);
					continue;
				}
				case 21: // JumpIfEqual addr
				{
					PC++;					

					if (AC == 0)
					{
						PC = readVal(PC, pipe1, pipe2);
						continue;
					}
					
					break;
				}
				case 22:
				{
					PC++;
	
					if (AC != 0)
					{
						PC = readVal(PC, pipe1, pipe2);
						continue;
					}

					break;
				}
				case 23:
				{
					PC += 2;
					writeVal(SP, PC, pipe1);
					SP--;
					PC--;
					PC = readVal(PC, pipe1, pipe2);
					continue;
				}
				case 24:
				{
					SP++;
					PC = readVal(SP, pipe1, pipe2);
					checkValidAddress(isUserMode, PC, childPid);
					continue;
				}
				case 25:
				{
					X++;
					break;
				}
				case 26:
				{
					X--;
					break;
				}
				case 27:
				{
					writeVal(SP, AC, pipe1);
					SP--;
					break;
				}
				case 28:
				{
					SP++;
					AC = readVal(SP, pipe1, pipe2);
					break;
				}
				case 29:
				{
					isUserMode = 0;	// Set CPU to kernel mode
					int tempSP = SP;
					int tempPC = PC;
					
					SP = 1999;
					PC = 1499;
					writeVal(SP, tempSP, pipe1);
					SP--;
					writeVal(SP, tempPC, pipe1);
					SP--;
					break;
				}
				case 30:
				{
					SP++;
					PC = readVal(SP, pipe1, pipe2);
					SP++;
					SP = readVal(SP, pipe1, pipe2);
					isUserMode = 1;	// Reset CPU to user mode
					break;
				}
				case 50: // End execution
				{
					shouldExit = 1;
					kill(childPid, SIGTERM);
					break;
				}
				default:
				{
					fprintf(stderr, "ERROR: Invalid instruction\n");
					kill(childPid, SIGTERM);
					exit(1);
				}

			} // End of switch case statement
			
			PC++;

		} // End while loop

		// If the something went wrong with the child, then waitppid caused an error
		if((pidReturned = waitpid(childPid, &status, 0)) == -1)
		{
			perror("waitPid");
			exit(1);
		}

	}

	exit(0);
} // End main
