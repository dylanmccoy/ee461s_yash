#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// inputs
// str - pointer to string to split
// delimiters - string of char to split by
// tokens - array to put the tokens into
// return value - # of tokens created
int split(char * str, char * delimiters, char ** tokens) {
	char * pch;
	int i = 0;
	pch = strtok(str, delimiters);
	while (pch != NULL) {
		*tokens = pch;
		i++;
		tokens++;
		pch = strtok (NULL, delimiters);
	}
	*tokens = NULL;
	return i;
}

int main() {

	printf("Hello, world!\n");

	int cpid = fork();
	char *args[]= {"date", NULL};
	if (cpid == 0) {
		execvp(args[0],args);
	} else {
		wait(NULL);
	}

	char input[2000];
	char * argv[10];
	while (1) {
		printf("#");
		fgets(input, sizeof(input), stdin);
		printf("string entered: %s", input);
		int len = split(input, " ", argv);
		printf("tokens found = %d\n", len);
		if (len > 0) {
			cpid = fork();
			if (cpid == 0) {
				printf("child\n");
				printf("execute %s", argv[0]);
				execvp(argv[0], args);
			} else {
				wait(NULL);
			}
		}
	}
}
