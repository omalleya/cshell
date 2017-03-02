#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#define MAX_COMMAND_LENGTH 40
#define MAX_PIDS         1000 // maximum PIDs to track

int signalNum = 0;
pid_t fgpid = INT_MAX;
pid_t bgpid[MAX_PIDS];         // array of open background process IDs
pid_t completed_pid[MAX_PIDS]; // array of completed bg process IDs
int cur=0;
int fgOnly = 0;

void runShell();
void parseCommand(char*, int*);
void checkCommand(char**, int, char*, char*, int*, int);
void runCommand(char**);
void changeDirectory(char**, int);
int executeShell(char**, char*, char*, int, int);
void checkBg();

// create instance of sigaction struct for background processes
struct sigaction stopFg_act = {0};

// create instance of sigaction struct for foreground processes
struct sigaction foreground_act = {0};

// create sigaction struct to ignore interrupts the rest of the time
struct sigaction restOfTheTime_act = {0};


void checkBg()
{
	printf("line 39\n");
	int i=0;
	printf("line 41\n");
	for(i=0; i<cur; i++)
	{
		printf("line 44\n");
		int childExitStatus=-5;
		printf("line 46\n");
		bgpid[i] = waitpid(bgpid[i], &childExitStatus, WNOHANG);
		//if process is done
		if(bgpid[i] == -1)
		{
			//pid has already finished and outputted it's exit status
		}else if(bgpid[i] != 0)
		{
			printf("line 54\n");
			if(WIFEXITED(childExitStatus))
				printf("background pid %d is done: exit value %d\n", bgpid[i], WEXITSTATUS(childExitStatus));
			else if (WIFSIGNALED(childExitStatus))
				printf("background pid %d is done: terminated by signal %d\n", bgpid[i], WTERMSIG(childExitStatus));
			else
				printf("Child did not terminate with exit\n");

		}
	}
}

void runShell() {

	printf("line 68\n");
	char* command = malloc(sizeof(char)*MAX_COMMAND_LENGTH);
	memset(command, '\0', MAX_COMMAND_LENGTH);
	int exitStatus = -5;

	printf("line 73\n");
	do {
		printf(": ");
		fflush(stdout);
		fgets(command, MAX_COMMAND_LENGTH, stdin);

		//parses command and then checks command for proper functionality
		printf("line 80\n");
		parseCommand(command, &exitStatus);
		checkBg();
		printf("line 83\n");

	}while(1);
}

void parseCommand(char* command, int* exitStatus)
{
	const char s[1] = " ";
	char* token;
	char* inputFile = NULL;
	char* outputFile = NULL;
	//set to one to add NULL
	int numArgs = 1;
	int i=0;
	int background=0;

	//need to copy command because strtok edits command string
	printf("line 100\n");
	char* temp = malloc(sizeof(char)*strlen(command));
	strcpy(temp, command);

	token = strtok(temp, s);

	//figure out how many args there are
	printf("line 107\n");
	while( token != NULL ) 
	{
		printf("line 110\n");
		if(strcmp(token, "<")==0)
		{
			token = strtok(NULL, s);
			inputFile = malloc(sizeof(char)*strlen(token));
			strcpy(inputFile,token);
			inputFile[strcspn(inputFile, "\n")] = 0;
			
		}else if(strcmp(token, ">")==0)
		{
			token = strtok(NULL, s);
			outputFile = malloc(sizeof(char)*strlen(token));
			strcpy(outputFile,token);
			outputFile[strcspn(outputFile, "\n")] = 0;
			
		}else if(strcmp(token, "&\n")==0)
		{
			token = strtok(NULL, s);
			//sets background variable to true
			if(fgOnly == 0)
			{
				background=1;
			}
			
		}else if(token != NULL) {
			numArgs++;
		}
		token = strtok(NULL, s);
	}
	printf("line 139\n");
	free(temp);

	//create args array with size figured out in while loop
	char **args = (char**) malloc((numArgs)*sizeof(char*));
	printf("line 144\n");

	token = strtok(command, s);
	for(i=0; i<numArgs-1; i++)
	{
		args[i] = malloc(sizeof(char)*strlen(token));
		strcpy(args[i], token);
		//removes new line character
		args[i][strcspn(args[i], "\n")] = 0;
		token = strtok(NULL, s);
	}
	printf("line 155\n");
	args[numArgs-1] = NULL;

	//set input and output files for background process
	if(background==1)
	{
		if(inputFile==NULL)
		{
			inputFile = malloc(sizeof(char)*12);
			strcpy(inputFile, "/dev/null");
		}
		if(outputFile==NULL)
		{
			outputFile = malloc(sizeof(char)*12);
			strcpy(outputFile, "/dev/null");
		}
	}
	printf("line 172\n");


	checkCommand(args, numArgs, inputFile, outputFile, exitStatus, background);
}

void checkCommand(char** args, int numArgs, char* inputFile, char* outputFile, int* exitStatus, int background)
{
	printf("line 180\n");
	if(strcmp(args[0],"cd")==0)
	{
		changeDirectory(args, numArgs);
	}else if(strcmp(args[0],"status")==0)
	{
		printf("exit value %d\n", *exitStatus);
		fflush(stdout);
	}else if(strcmp(args[0],"exit")==0)
	{
		int i=0;
		for(i=0; i<cur; i++)
		{
			if(bgpid[i] == 0)
			{
				kill(bgpid[i], SIGKILL);
			}
		}
		for(i=0; i<numArgs; i++)
		{
			free(args[i]);
		}
		printf("line 202\n");
		exit(0);
	}else if(args[0][0] == '#'||strcmp(args[0],"")==0)
	{
		//comment line so do nothing
	}else {
		//exec shell
		printf("line 209\n");
		*exitStatus = executeShell(args, inputFile, outputFile, numArgs, background);
	}
	printf("line 212\n");
	free(inputFile);
	free(outputFile);
	printf("line 215\n");
}

void changeDirectory(char** args, int numArgs)
{
	char currentDir[100];

	if(numArgs > 2)
	{
		if(chdir(args[1]) == -1)
		{
			printf("Couldn't change directory.\n");
			fflush(stdout);
		}else {
			printf("Changed directory to: %s\n", args[1]);
			getcwd(currentDir, sizeof(currentDir));
			printf("%s\n", currentDir);
			fflush(stdout);
		}
	}else {
		//go to home directory
		chdir(getenv("HOME"));
		getcwd(currentDir, sizeof(currentDir));
		printf("%s\n", currentDir);
		fflush(stdout);
	}
	printf("line 241\n");
}

int executeShell(char** args, char* inputFile, char* outputFile, int numArgs, int background)
{
	pid_t spawnPid = -5;
	int childExitStatus = -5;
	printf("line 248\n");
	spawnPid = fork();
	switch (spawnPid) {
		case -1: { 
			perror("Hull Breach!\n"); 
			exit(1); 
			break; 
		}
		case 0: {
			//necessary redirection
			int sourceFD, targetFD, result;
			printf("line 259\n");
			if(inputFile!=NULL)
			{
				sourceFD = open(inputFile, O_RDONLY);
				if (sourceFD == -1) { perror("source open()"); exit(1); }
				result = dup2(sourceFD, 0);
				if (result == -1) { perror("source dup2()"); exit(2); }
				fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
			}

			if(outputFile!=NULL)
			{
				targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0744);
				if (targetFD == -1) { perror("target open()"); exit(1); }
				result = dup2(targetFD, 1);
				if (result == -1) { perror("target dup2()"); exit(2); }
				fcntl(targetFD, F_SETFD, FD_CLOEXEC);
			}

			printf("line 278\n");
			execvp(args[0], args);
			exit(2); 
			break;
		}
		default: {
			if(background == 0)
			{
				//pid_t actualPid = waitpid(spawnPid, &childExitStatus, 0);
				signalNum = 0;
				fgpid = spawnPid;

				// set interrupt handler for fg process 
                sigaction(SIGINT, &foreground_act, NULL);

				// wait for fg child process
                fgpid = waitpid(fgpid, &childExitStatus, 0);

				// restore to ignore interrupts
                sigaction(SIGINT, &restOfTheTime_act, NULL);

				fgpid = INT_MAX;

				printf("line 301\n");

				// if process was terminated by signal, print message
				if (signalNum != 0)
				{
					printf("terminated by signal %d\n", signalNum);
				}

			}else{
				printf("line 310\n");
				pid_t childPID = waitpid(childPID, &childExitStatus, WNOHANG);
				bgpid[cur++] = childPID;
				printf("background pid is %d\n", spawnPid);
			}
			break;

			
		}
	}

	printf("line 321\n");
	int i=0;
	for(i=0; i<numArgs; i++)
	{
		free(args[i]);
	}
	free(args);
	printf("line 328\n");

	return childExitStatus;
}

void fgHandler()
{

	printf("line 336\n");
	if(fgOnly == 0)
	{
		printf("Entering foreground-only mode (& is now ignored)\n");
		fgOnly = 1;
	}else {
		printf("Exiting foreground-only mode\n");
		fgOnly = 0;
	}
	printf("line 345\n");

    return;
}



void sigintHandler()
{
	printf("line 354\n");
    // if interrupt signal occurs while fg process is running, kill it
    if (fgpid != INT_MAX)
    {
		printf("%d\n", fgpid);	
        // kill the foreground process
        kill(fgpid, SIGKILL);
 
        // set global variable for status messages
        signalNum = 2;  
    }  
	printf("line 365\n");

    // ignore interrupt signal for all other processes
    // and simply return
    return;
}

int main() {

	printf("line 374\n");
	stopFg_act.sa_handler = fgHandler;
	stopFg_act.sa_flags = SA_RESTART;
	sigfillset(&(stopFg_act.sa_mask));
	sigaction(SIGTSTP, &stopFg_act, NULL);

	foreground_act.sa_handler = sigintHandler;
	foreground_act.sa_flags = SA_RESTART;
	sigfillset(&(foreground_act.sa_mask));
	sigaction(SIGINT, &foreground_act, NULL); 

	restOfTheTime_act.sa_handler = SIG_IGN;
	restOfTheTime_act.sa_flags = SA_RESTART;
	sigfillset(&(restOfTheTime_act.sa_mask));
	sigaction(SIGINT, &restOfTheTime_act, NULL); 
	printf("line 389\n");

	runShell();	
	printf("final\n");
	return 0;

}
