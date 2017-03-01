#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COMMAND_LENGTH 40

void runShell();
void parseCommand(char*);
void checkCommand(char**, int);
void runCommand(char**);
void changeDirectory(char**, int);

void runShell() {

	char* command = malloc(sizeof(char)*MAX_COMMAND_LENGTH);

	do {
		printf(": ");
		fflush(stdout);
		fgets(command, MAX_COMMAND_LENGTH, stdin);

		//check command for proper functionality
		parseCommand(command);

	}while(strcmp(command, "exit\n") != 0);
}

void parseCommand(char* command)
{
	const char s[1] = " ";
	char* token;
	//set to one to add NULL
	int numArgs = 1;
	int i=0;

	//need to copy command because strtok edits command string
	char* temp = malloc(sizeof(char)*strlen(command));
	strcpy(temp, command);

	token = strtok(temp, s);

	//figure out how many args there are
	while( token != NULL ) 
	{
		numArgs++;
		token = strtok(NULL, s);
	}

	//create args array with size figured out in while loop
	char **args = (char**) malloc((numArgs)*sizeof(char*));

	token = strtok(command, s);
	for(i=0; i<numArgs-1; i++)
	{
		args[i] = malloc(sizeof(char)*strlen(token));
		strcpy(args[i], token);
		//removes new line character
		args[i][strcspn(args[i], "\n")] = 0;
		printf("args: %s\n", args[i]);
		token = strtok(NULL, s);
	}
	args[numArgs-1] = NULL;


	checkCommand(args, numArgs);
}

void checkCommand(char** args, int numArgs)
{
	if(strcmp(args[0],"cd")==0)
	{
		changeDirectory(args, numArgs);
	}else if(strcmp(args[0],"status")==0)
	{
		printf("getting status\n");
	}else if(strcmp(args[0],"exit")==0)
	{
		printf("exiting\n");
	}else if(strcmp(args[0],"#")==0)
	{
		//comment line so do nothing
	}else {
		//exec shell
		pid_t spawnPid = -5;
		int childExitStatus = -5;
		spawnPid = fork();
		switch (spawnPid) {
			case -1: { 
				perror("Hull Breach!\n"); 
				exit(1); 
				break; 
			}
			case 0: {
				printf("CHILD(%d): Sleeping for 1 second\n", getpid());
				sleep(1);
				printf("CHILD(%d): Converting into \'ls -a\'\n", getpid());
				execvp(args[0], args);
				perror("CHILD: exec failure!\n");
				exit(2); 
				break;
			}
			default: {
				printf("PARENT(%d): Sleeping for 2 seconds\n", getpid());
				sleep(2);
				printf("PARENT(%d): Wait()ing for child(%d) to terminate\n", getpid(), spawnPid);
				pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0);
				printf("PARENT(%d): Child(%d) terminated, Exiting!\n", getpid(), actualPid);
				exit(0); break;
			}
		}
	}
}

void changeDirectory(char** args, int numArgs)
{
	char currentDir[100];

	printf("changing directory\n");
	if(numArgs > 2)
	{
		if(chdir(args[1]) == -1)
		{
			printf("Couldn't change directory.\n");
		}else {
			printf("Changed directory to: %s\n", args[1]);
			getcwd(currentDir, sizeof(currentDir));
			printf("%s\n", currentDir);
		}
	}else {
		//go to home directory
		chdir(getenv("HOME"));
		getcwd(currentDir, sizeof(currentDir));
		printf("%s\n", currentDir);
	}
}

int main() {

	runShell();	
	return 0;
}
