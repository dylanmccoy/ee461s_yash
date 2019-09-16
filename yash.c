#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h> 
#include <fcntl.h>
#include <ctype.h>


#define E_SUCCES 0
#define E_FAILURE 1
#define E_SYN_PIPE 2
#define E_SYN_BG 3
#define E_SYN_REDIR 4
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

int file_redir(char * out, char * in, char * err) {
	if (strcmp(out, "") != 0) {
		printf("out redirected to %s\n", out);
		int fd = open(out, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		dup2(fd, 1); // stdout is file descriptor 1
		close(fd);
	}
	if (strcmp(in, "") != 0) {
		printf("in redirected to %s\n", in);
		int fd = open(in, O_RDONLY);
		dup2(fd, 0); // stdin is fd 0
		close(fd);
	}
	if (strcmp(err, "") != 0) {
		printf("err redirected to %s\n", err);
		int fd = open(err, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		dup2(fd, 2);
		close(fd);
	}
	return E_SUCCES;
}

int run_cmds 
(
	char ** argv1, 
	char ** argv2, 
	int len_argv1, 
	int len_argv2,
	char * out, 
	char * in, 
	char * err, 
	bool bg
) {
	int errno = E_SUCCES;
	int     fd[2], nbytes;
    pid_t   cpid1, cpid2;
	pipe(fd);
	if (len_argv2 > 0) {		// need to create a pipe
		pipe(fd);
		if((cpid1 = fork()) == -1) {
			printf("error when fork\n");
    	}
		if (cpid1 == 0) {		// code for child1

		} else {
			if ((cpid2 = fork()) == -1) {
				printf("error when fork\n");
			}
			if (cpid2 == 0) {	// code for child2
				errno = file_redir(out, in, err);
			} else {			// code for yash

			}
		}
	} else {					// no need for pipe, only one cmd
		if ((cpid1 = fork()) == -1) {
			printf("error when fork\n");
		}
		if (cpid1 == 0) {		// code for child
			errno = file_redir(out, in, err);
			execvp(argv1[0], argv1);
		} else {				// code for yash
			wait(NULL);
		}
	}
	// only yash should really reach here, as execvp will have been called for each of the children
	for (int i = 0; i < len_argv1; i++) {
		printf("argv1[%d] = %s\n", i, argv1[i]);
	}
	for (int i = 0; i < len_argv2; i++) {
		printf("argv2[%d] = %s\n", i, argv2[i]);
	}
	if (bg) {
		printf("job to run in bg\n");
	}	
	return errno; 
}

int parse_tokens_and_run(char ** tokens, int len_tokens) {
	// up to 2 possible commands to be executed
	int errno = E_SUCCES;
	int len_args1 = 0, len_args2 = 0;
	char ** cur_args1 = (char **) malloc(100 * sizeof(char *));
	char ** cur_args2 = (char **) malloc(100 * sizeof(char *));
	char * output = "";
	char * input = "";
	char * err = "";
	bool bg = false;
	int cmd = 1;
	// for (int i = 0; i < len_tokens; i++) {
	// 	printf("argv1[%d] = %s\n", i, tokens[i]);
	// }

	for (int i = 0; i < len_tokens; i++) {
		if (strcmp(tokens[i], ">") == 0) {
			if (i == len_tokens - 1) {
				printf("yash: syntax error near unexpected token 'newline'\n");
				errno = E_SYN_REDIR;
				break;
			} else if (!isalpha(tokens[i + 1][0]) && !isdigit(tokens[i + 1][0])) {
					printf("syntax error, filename expected\n");
					errno = E_SYN_REDIR;
					break;
			} else {

				output = tokens[i + 1];
				cmd = 0;
			}
			// use next token as output
		} else if (strcmp(tokens[i], "<") == 0) {
			if (i == len_tokens - 1) {
				printf("yash: syntax error near unexpected token 'newline'\n");
				errno = E_SYN_REDIR;
				break;
			} else if (!isalpha(tokens[i + 1][0]) && !isdigit(tokens[i + 1][0])) {
					printf("syntax error, filename expected\n");
					errno = E_SYN_REDIR;
					break;
			} else {
				input = tokens[i + 1];
				cmd = 0;
			}
			// use next token as input
		} else if (strcmp(tokens[i], "2>") == 0) {
			if (i == len_tokens - 1) {
				printf("yash: syntax error near unexpected token 'newline'\n");
				errno = E_SYN_REDIR;
				break;
			} else if (!isalpha(tokens[i + 1][0]) && !isdigit(tokens[i + 1][0])) {
					printf("syntax error, filename expected\n");
					errno = E_SYN_REDIR;
					break;
			} else {
				err = tokens[i + 1];
				cmd = 0;
			}
			// use next token as stderr
		} else if (strcmp(tokens[i], "|") == 0) {
			if (i == len_tokens - 1) {
				printf("| found unexpectedly\n");
				errno = E_SYN_PIPE;
				break;
			} else if (len_args1 == 0) {
				printf("expected command before pipe\n");	
				errno = E_SYN_PIPE;
				break;
			} else {
				if (cmd == 0) {
					printf("| found unexpectedly\n");
					errno = E_SYN_PIPE;
					break;
				} else {
					cmd = 2;
				}
			}
			// use previous cmd as input to next cmd
		} else if (strcmp(tokens[i], "&") == 0) {
			if (i != len_tokens - 1) {
				printf("& found unexpectedly\n");
				errno = E_SYN_BG;
				break;
			} else {
				bg = true;
				// put program in background
			}
		} else {
			if (cmd == 1) {			// cmd determines which args the token is part of
				printf("adding to cmd1\n");
				cur_args1[len_args1] = tokens[i];
				len_args1++;
			} else if (cmd == 2){
				printf("adding to cmd2\n");
				cur_args2[len_args2] = tokens[i];
				len_args2++;
			}
		}
	}
	if (errno == 0 && len_args1 > 0) {
		cur_args1[len_args1] = NULL;		// append NULL to each for use for execvp later on
		cur_args2[len_args2] = NULL;
		errno = run_cmds (
			cur_args1, cur_args2,
			len_args1, len_args2,
			output, input, err,
			bg
		);
	}

	return errno;
}

int main() {
	int cpid1, cpid2;
	char input[2000];
	char * argv[100];
	while (1) {
		int input_len = get_input(input);
		int token_len = split(input, " ", argv);
		printf("tokens found = %d\n", token_len);
		// for (int j = 0; j < len; j++) {
		// printf("%s\n", argv[j]);
		// }
		if (token_len > 0) {
			parse_tokens_and_run(argv, token_len);
		}
	}
}
