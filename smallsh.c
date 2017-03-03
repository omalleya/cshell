#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

#define MAX_COMMAND_LENGTH 2048
#define MAX_PIDS         1000 // maximum PIDs to track

int signalNum = 0;
pid_t fgpid = INT_MAX;
pid_t bgpid[MAX_PIDS];         // array of open background process IDs
int cur=0;
int fgOnly = 0;

//runs looping shell
void runShell();
//parses user input correctly
void parseCommand(char*, int*);
//finds out how to run command
void checkCommand(char**, int, char*, char*, int*, int);
//splits off command based on args array
void runCommand(char**);
//changes directory
void changeDirectory(char**, int);
//executes shell after forking child process
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
	int i=0;
	for(i=0; i<cur; i++)
	{
		int childExitStatus=-5;
		bgpid[i] = waitpid(bgpid[i], &childExitStatus, WNOHANG);
		//if process is done
		if(bgpid[i] == -1)
		{
			//pid has already finished and outputted it's exit status
		}else if(bgpid[i] != 0)
		{
			//pid has just finished so lets print the exit status
			if(WIFEXITED(childExitStatus))
				printf("background pid %d is done: exit value %d\n", bgpid[i], WEXITSTATUS(childExitStatus));
			else if (WIFSIGNALED(childExitStatus))
				printf("background pid %d is done: terminated by signal %d\n", bgpid[i], WTERMSIG(childExitStatus));
			else
				printf("Child did not terminate with exit\n");
			fflush(stdout);

		}
	}
}

void runShell() {

	char* command = malloc(sizeof(char)*MAX_COMMAND_LENGTH);
	memset(command, '\0', MAX_COMMAND_LENGTH);
	int exitStatus = -5;

	do {
		printf(": ");
		fflush(stdout);
		fflush(stdin);
		//get command from user
		fgets(command, MAX_COMMAND_LENGTH, stdin);

		//parses command and then checks command for proper functionality
		parseCommand(command, &exitStatus);
		//checks background processes for completion
		checkBg();
		free(command);
		command = malloc(sizeof(char)*MAX_COMMAND_LENGTH);
		memset(command, '\0', MAX_COMMAND_LENGTH);

	}while(1);

	free(command);
}

void parseCommand(char* command, int* exitStatus)
{
	const char s[1] = " ";
	char* token = NULL;
	char* inputFile = NULL;
	char* outputFile = NULL;
	//set to one to add NULL
	int numArgs = 1;
	int i=0;
	int background=0;

	char strC[40];
	int n;
	char strTemp[15];

	//replaces $$ with pid
	for(i=0; i<strlen(command)-1; i++)
	{
		if(command[i]=='$' && command[i+1]=='$')
		{
			strncpy(strC,command,i);
			strC[i] = '\0';
			//get pid as string
			n = sprintf(strTemp, "%ld", (long)getpid());
			//concatenate to command
			strcat(strC, strTemp);
			//concatenate rest of command onto string
			strcat(strC,command+(i+2));
			printf("%s\n",strC);
			fflush(stdout);
			strcpy(command, strC);
		}
	}

	

	//need to copy command because strtok edits command string
	char* temp = calloc(strlen(command)+1, sizeof(char));
	strcpy(temp, command);

	token = strtok(temp, s);

	//figure out how many args there are
	while( token != NULL ) 
	{
		if(strcmp(token, "<")==0)
		{
			//get value after <
			token = strtok(NULL, s);
			//set inputFile to that value
			inputFile = calloc(strlen(token)+1, sizeof(char));
			strcpy(inputFile,token);
			inputFile[strcspn(inputFile, "\n")] = 0;
			
		}else if(strcmp(token, ">")==0)
		{
			//get value after >
			token = strtok(NULL, s);
			//set outputFile to that value
			outputFile = calloc(strlen(token)+1, sizeof(char));
			strcpy(outputFile,token);
			outputFile[strcspn(outputFile, "\n")] = 0;
			
		}else if(strcmp(token, "&\n") == 0)
		{
			token = strtok(NULL, s);
			//sets background variable to true if we're not in foreground only mode
			if(fgOnly == 0)
			{
				background=1;
			}
			
		}else if(token != NULL) {
			numArgs++;
		}

		token = strtok(NULL, s);
	}
	free(temp);

	//create args array with size figured out in while loop
	char **args = calloc(numArgs, sizeof(char*));

	token = strtok(command, s);
	for(i=0; i<numArgs-1; i++)
	{
		if(token==NULL)
		{
			continue;
		}else if(token[0] == '&' && strcmp(args[0],"echo") != 0)
		{
			//double check that we aren't sending & to exec or using it in foreground only mode
			if(fgOnly == 0)
			{
				background=1;
			}
			continue;
		}
		args[i] = calloc(strlen(token)+1, sizeof(char));
		strcpy(args[i], token);
		//removes new line character
		args[i][strcspn(args[i], "\n")] = 0;
		token = strtok(NULL, s);
	}
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


	checkCommand(args, numArgs, inputFile, outputFile, exitStatus, background);
	free(inputFile);
	free(outputFile);
	for(i=0; i<numArgs; i++)
	{
		free(args[i]);
	}
	free(args);
}

void checkCommand(char** args, int numArgs, char* inputFile, char* outputFile, int* exitStatus, int background)
{
	if(strcmp(args[0],"cd")==0)
	{
		changeDirectory(args, numArgs);
	}else if(strcmp(args[0],"status")==0)
	{
		printf("exit value %d\n", *exitStatus);
		fflush(stdout);
	}else if(strcmp(args[0],"exit")==0)
	{
		//kill bg processes
		int i=0;
		for(i=0; i<cur; i++)
		{
			if(bgpid[i] == 0)
			{
				kill(bgpid[i], SIGKILL);
			}
		}
		//free args variables
		for(i=0; i<numArgs; i++)
		{
			free(args[i]);
		}
		exit(0);
	}else if(args[0][0] == '#'||strcmp(args[0],"")==0)
	{
		//comment line so do nothing
	}else {
		//exec shell
		*exitStatus = executeShell(args, inputFile, outputFile, numArgs, background);
	}
}

void changeDirectory(char** args, int numArgs)
{
	char currentDir[100];

	if(numArgs > 2)
	{
		//change dir in this if clause
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
}

int executeShell(char** args, char* inputFile, char* outputFile, int numArgs, int background)
{
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
			//necessary redirection for input
			int sourceFD, targetFD, result;
			if(inputFile!=NULL)
			{
				sourceFD = open(inputFile, O_RDONLY);
				if (sourceFD == -1) { perror("source open()"); exit(1); }
				result = dup2(sourceFD, 0);
				if (result == -1) { perror("source dup2()"); exit(2); }
				fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
			}
			//necessary redirection for output
			if(outputFile!=NULL)
			{
				targetFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0744);
				if (targetFD == -1) { perror("target open()"); exit(1); }
				result = dup2(targetFD, 1);
				if (result == -1) { perror("target dup2()"); exit(2); }
				fcntl(targetFD, F_SETFD, FD_CLOEXEC);
			}

			//call exec in this if clause if proper conditions met
			if(strcmp(args[0],"")!=0||args[0]==NULL)
			{
				if (execvp(args[0], args) < 0) {
					perror("error");
					fflush(stdout);
					exit(2);
				} else {
					return execvp(args[0], args);
				}
			}
			break;
		}
		default: {
			if(background == 0)
			{
				signalNum = 0;
				fgpid = spawnPid;

				//set interrupt handler for fg process 
                sigaction(SIGINT, &foreground_act, NULL);

				//wait for fg child process
                fgpid = waitpid(fgpid, &childExitStatus, 0);

				//restore to ignore interrupts
                sigaction(SIGINT, &restOfTheTime_act, NULL);

				fgpid = INT_MAX;


				//if process was terminated by signal, print message
				if (signalNum != 0)
				{
					printf("terminated by signal %d\n", signalNum);
					fflush(stdout);
				}

			}else{
				pid_t childPID = INT_MAX;
				childPID = waitpid(childPID, &childExitStatus, WNOHANG);
				//add this process to bg process list
				bgpid[cur++] = childPID;
				//output current pid
				printf("background pid is %d\n", spawnPid);
				fflush(stdout);
			}
			break;

			
		}
	}

	//return with proper exit status
	if(WIFEXITED(childExitStatus))
		return WEXITSTATUS(childExitStatus);
	return WTERMSIG(childExitStatus);
}

void fgHandler()
{

	//if we're not in fg only, print we're entering and set fgOnly to 1
	//else we're in fg only, print we're exiting and set fgOnly to 0
	if(fgOnly == 0)
	{
		puts("Entering foreground-only mode (& is now ignored)\n");
		fgOnly = 1;
	}else {
		puts("Exiting foreground-only mode\n");
		fgOnly = 0;
	}

    return;
}



void sigintHandler()
{
    //if interrupt signal occurs while fg process is running, kill it
    if (fgpid != INT_MAX)
    {
        kill(fgpid, SIGKILL);
 
        //set global variable for status messages
        signalNum = 2;  
    }  
    return;
}

int main() {

	//set up fg only mode signal handler
	stopFg_act.sa_handler = fgHandler;
	stopFg_act.sa_flags = SA_RESTART;
	sigfillset(&(stopFg_act.sa_mask));
	sigaction(SIGTSTP, &stopFg_act, NULL);

	//set up kill fg process signal handler
	foreground_act.sa_handler = sigintHandler;
	foreground_act.sa_flags = SA_RESTART;
	sigfillset(&(foreground_act.sa_mask));
	sigaction(SIGINT, &foreground_act, NULL); 

	//ignore all other signals signal handler
	restOfTheTime_act.sa_handler = SIG_IGN;
	restOfTheTime_act.sa_flags = SA_RESTART;
	sigfillset(&(restOfTheTime_act.sa_mask));
	sigaction(SIGINT, &restOfTheTime_act, NULL); 

	//run the shell
	runShell();	
	return 0;

}
