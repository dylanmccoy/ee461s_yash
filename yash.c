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

int get_input(char * input) {
	printf("#");
	fgets(input, 1000, stdin);
	size_t s_len = strlen(input);
	input[s_len-1] = 0;			// remove \n from input
	printf("string entered: %s\n", input);
	return s_len;
}

int parse_tokens(char ** tokens, int len_tokens) {
	for (int i = 0; i < len_tokens; i++) {
		if (strcmp(tokens[i], ">") == 0) {

		} else if (strcmp(tokens[i], "<") == 0) {

		} else if (strcmp(tokens[i], "2>") == 0) {

		} else if (strcmp(tokens[i], "|") == 0) {

		} else if (strcmp(tokens[i], "&") == 0) {
			if (i != len_tokens - 1) {
				printf("& found unexpectedly");
			}
		}
	}
}

int main() {
	int cpid1, cpid2;
	char input[2000];
	char * argv[100];
	while (1) {
		int input_len = get_input(input);
		int len = split(input, " ", argv);
		printf("tokens found = %d\n", len);
		// for (int j = 0; j < len; j++) {
		// printf("%s\n", argv[j]);
		// }
		if (len > 0) {
			cpid1 = fork();
			if (cpid1 == 0) {
				// printf("child process with args\n");
				// for (int i = 0; i < len; i++) {
				// 	printf("argv[%d] = %s\n", i, argv[i]);
				// }
				execvp(argv[0], argv);
				// execvp(args[0], args);
			} else {
				wait(NULL);
			}
		}
	}
}
