#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 40

void runShell();

void runShell() {

	char* command = malloc(sizeof(char)*MAX_COMMAND_LENGTH);

	do {
		printf(": ");
		fgets(command, MAX_COMMAND_LENGTH, stdin);

		//test print of command
		printf("%s", command);

	}while(strcmp(command, "exit\n") != 0);
}

int main() {

	runShell();	
	return 0;
}
