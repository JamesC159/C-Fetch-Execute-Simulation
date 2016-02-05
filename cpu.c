#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>		// May be useless, IDK yet.
#include <string.h>

int main (int arc, char **argv)
{

	int pipe1[2];		// File descriptors for PC writing to memory
	int pipe2[2];		// File descriptors for PC Reading from memory
	pid_t childPid;		// Child Process ID
	int status;			// Status returned from waiting for the child to finish

	pid_t cPid = getpid();

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

	switch(childPid)
	{
		case -1:
		{
			perror("childPid");
			exit(1);
		}
		case 0:		//Child = Memory
		{
			int mem[2000];		// Memory that holds 2000 integers that represent instructions

			// File names to use as tests
			const char *sample1 = "sample1.txt";
			FILE *file;									// File to be read into memory array
			char *str = malloc(sizeof(char) * 50);		// Contains strings from the file
			int numRead = 100;							// Number of bytes to read from the file
			int num = 0;								// Actual instruction to reside in memory
			int idx = 0;								// Index into memory
			int count = 0;								// How many instructions were read into memory

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
				char *token = malloc (sizeof(char) * 50);
				char *delims = " ";
				token = strtok(str, delims);	// We really dont care about destroyign the rest of the string
												// because the rest doest matter after the integer

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

			// Read the PC from the processor and fetch the instruction at that location from memory
			read(pipe1[0], &idx, sizeof(int));
			write(pipe2[1], &(mem[idx]), sizeof(int));

			_exit(0);
		}
		default:	//Parent = CPU
		{
			//CPU Registers
			int PC = 0;
			int SP = 0;
			int IR = 0;
			int AC = 0;
			int X = 0;
			int Y = 0;
			int instr = 0;
			int pidReturned = 0;

			// Create pipes for CPU to Mem communication
			close(pipe1[0]);
			close(pipe2[1]);
			dup2(pipe1[1], STDOUT_FILENO);
			dup2(pipe2[0], STDIN_FILENO);

			// Send the PC to memory to fetch an instruction
			write(pipe1[1], &PC, sizeof(PC));
			read(pipe2[0], &instr, sizeof(int));

			fprintf(stderr, "Instruction read from memory is: %d\n", instr);

			// If the something went wrong with the child, then waippid caused an error
			if((pidReturned = waitpid(childPid, &status, 0)) == -1)
			{
				perror("waitPid");
				exit(1);
			}

			break;
		}	
	}

	exit(0);
}