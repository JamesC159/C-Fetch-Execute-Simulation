#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>		// May be useless, IDK yet.
#include <string.h>
#include <limits.h>
#include <math.h>

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
		const char *sample1 = "sample1.txt";
		
		FILE *file;	// File to be read into memory array
		char *str = malloc(sizeof(char) * 50);	// Contains strings from the file
		int numRead = 100;	// Number of bytes to read from the file
		int num = 0;		// Actual instruction to reside in memory
		int idx = 0;		// Index into memory
			
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
			
			// Check to see if we need to change the location of PC in memory
			if (token[0] == '.')
			{
				token++;
				idx = atoi(token);
			}

			// If there is an empty line in the file, ignore it
			else if (token[0] == '\n')
			{
				continue;
			}
			else
			{
				num = atoi(token);
				mem[idx++] = num;
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
			// Read the PC from the CPU  and fetch the instruction at that location from memory
			read(pipe1[0], &idx, sizeof(int));

			// Write instruction from memory to pipe for CPU to read
			write(pipe2[1], &(mem[idx]), sizeof(int));
		}

		_exit(0);
	}
	else 	//Parent = CPU
	{
		//CPU Registers
		int PC = 0;
		int SP = 0;
		int IR = 0;
		int AC = 0;
		int X = 0;
		int Y = 0;

		int pidReturned = 0;
		int isUsingUserStack = 1;	// Initially true
		int shouldExit = 0;	// Used to signal break from main execution loop

		// Create pipes for CPU to Mem communication
		close(pipe1[0]);
		close(pipe2[1]);
		dup2(pipe1[1], STDOUT_FILENO);
		dup2(pipe2[0], STDIN_FILENO);

		while (!shouldExit)
		{
			// Send the PC to memory to fetch an instruction
			write(pipe1[1], &PC, sizeof(PC));

			// Read the instruction written back to the CPU and store it in instr
			read(pipe2[0], &IR, sizeof(IR));

			// Switch block to determine which instruction needs to execute
			switch (IR)
			{
				case 1:	// Load value
				{

					PC++;

					// Fetch the value from memory
					write(pipe1[1], &PC, sizeof(PC));

					// Read the value written back to the CPU and store it in the AC
					read(pipe2[0], &AC, sizeof(AC));

					break;
				}
				case 2: // Load addr
				{
					break;
				}
				case 3:	// LoadInd addr
				{
					break;
				}
				case 4:	// LoadIdxX addr
				{
					PC++;					

					// Fetch address from memory
					write(pipe1[1], &PC, sizeof(PC));
					read(pipe2[0], &AC, sizeof(PC));
					
					AC += X;
					
					// Fetch address at AC + X form memory
					write(pipe1[1], &AC, sizeof(AC));
					read(pipe2[0], &AC, sizeof(AC));

					break;
				}
				case 5:
				{
					PC++;
					write(pipe1[1], &PC, sizeof(PC));
					read(pipe2[0], &AC, sizeof(AC));
					
					AC += Y;

					write(pipe1[1], &AC, sizeof(AC));
					read(pipe2[0], &AC, sizeof(AC));

					break;
				}
				case 6:
				{
					break;
				}
				case 7:
				{
					break;
				}
				case 8:
				{
					break;
				}
				case 9: // Put port
				{
					PC++;
					
					int port;
					write(pipe1[1], &PC, sizeof(PC));
					read(pipe2[0], &port, sizeof(port));
					
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
					break;
				}
				case 11:
				{
					AC += Y;
					break;
				}
				case 12:
				{
					break;
				}
				case 13:
				{
					break;
				}
				case 14:
				{						
					X = AC;
					fprintf(stderr, "Value in X after 14 is: %d\n", X);
					break;
				}
				case 15:
				{
					break;
				}
				case 16:
				{
					Y = AC;
					break;
				}
				case 17:
				{
					break;
				}
				case 18:
				{
					break;
				}
				case 19:
				{
					break;
				}
				case 20:
				{
					PC++;
					write(pipe1[1], &PC, sizeof(PC));
					read(pipe2[0], &PC, sizeof(PC));
					continue;
				}
				case 21: // JumpIfEqual addr
				{
					PC++;					

					if (AC == 0)
					{
						write(pipe1[1], &PC, sizeof(PC));
						read(pipe2[0], &PC, sizeof(AC));
						continue;
					}
					
					break;
				}
				case 22:
				{
					break;
				}
				case 23:
				{
					break;
				}
				case 24:
				{
					break;
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
					break;
				}
				case 28:
				{
					break;
				}
				case 29:
				{
					break;
				}
				case 30:
				{
					break;
				}
				case 50: // End execution
				{
					shouldExit = 1;
					kill (childPid, SIGTERM);
					break;
				}
				default:
				{
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
