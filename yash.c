#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h> 
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>


#define E_SUCCES 0
#define E_FAILURE 1
#define E_SYN_PIPE 2
#define E_SYN_BG 3
#define E_SYN_REDIR 4

static void sig_handler(int signo) { 
	switch(signo){ 
		case SIGINT:
			printf("caught SIGINT\n");
			exit(0);
			break; 
		case SIGTSTP:
			printf("caught SIGTSTP\n");
			break; 
	}
}

void handler(int sig)
{	
  pid_t pid;

  pid = wait(NULL);

  printf("Pid %d exited.\n", pid);
}
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

int get_input(char * in1) {
	printf("#");
	fgets(in1, 1000, stdin);
	size_t s_len = strlen(in1);
	in1[s_len-1] = 0;			// remove \n from in1
	// printf("string entered: %s\n", in1);
	return s_len;
}

int file_redir(char * out, char * in, char * err1) {
	if (strcmp(out, "") != 0) {
		// printf("out redirected to %s\n", out);
		int fd = open(out, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		dup2(fd, 1); // stdout is file descriptor 1
		close(fd);
	}
	if (strcmp(in, "") != 0) {
		// printf("in redirected to %s\n", in);
		int fd = open(in, O_RDONLY);
		if (fd != ENOENT) {
			dup2(fd, 0); // stdin is fd 0
		}
		close(fd);
	}
	if (strcmp(err1, "") != 0) {
		// printf("err1 redirected to %s\n", err1);
		int fd = open(err1, O_WRONLY|O_CREAT|O_TRUNC, 0666);
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
	char * out1, 
	char * in1, 
	char * err1,
	char * out2,
	char * in2,
	char * err2,
	bool bg
) {
	int _errno = E_SUCCES;
	int     fd[2], nbytes;
    pid_t   cpid1, cpid2;
	if (len_argv2 > 0) {		// need to create a pipe
		if (pipe(fd) < 0) {
			printf("pipe error\n");
		}
		if((cpid1 = fork()) == -1) {
			printf("error when fork\n");
    	}
		if (cpid1 == 0) {		// code for child1
			close(fd[0]);		// close in1 for child
			// dup2(fd[1], 1);
			close(fd[1]);
			_errno = file_redir(out1, in1, err1);
			// sleep(10);
			execvp(argv1[0], argv1);
		} else {
			// sleep(2);
			if ((cpid2 = fork()) == -1) {
				printf("error when fork\n");
			}
			if (cpid2 == 0) {	// code for child2
				close(fd[1]);		// close out for child
				// dup2(fd[0], 0);
				close(fd[0]);
				_errno = file_redir(out2, in2, err2);
				execvp(argv2[0], argv2);
			} else {			// code for yash
				// wait(NULL);
				// printf("parent waiting");
				// wait(NULL);
				waitpid(cpid2, NULL, WUNTRACED | WNOHANG);
			}
		}
	} else {					// no need for pipe, only one cmd
		if ((cpid1 = fork()) == -1) {
			printf("error when fork\n");
		}
		if (cpid1 == 0) {		// code for child
			_errno = file_redir(out1, in1, err1);
			execvp(argv1[0], argv1);
		} else {				// code for yash
			wait(NULL);
		}
	}
	// only yash should really reach here, as execvp will have been called for each of the children
	// for (int i = 0; i < len_argv1; i++) {
	// 	printf("argv1[%d] = %s\n", i, argv1[i]);
	// }
	// for (int i = 0; i < len_argv2; i++) {
	// 	printf("argv2[%d] = %s\n", i, argv2[i]);
	// }
	// if (bg) {
	// 	printf("job to run in bg\n");
	// }	
	return _errno; 
}

int parse_tokens_and_run(char ** tokens, int len_tokens) {
	// up to 2 possible commands to be executed
	int _errno = E_SUCCES;
	int len_args1 = 0, len_args2 = 0;
	char ** cur_args1 = (char **) malloc(100 * sizeof(char *));
	char ** cur_args2 = (char **) malloc(100 * sizeof(char *));
	char * out1 = "";
	char * in1 = "";
	char * err1 = "";
	char * out2 = "";
	char * in2 = "";
	char * err2 = "";
	char ** out = &out1;
	char ** in = &in1;
	char ** err = &err1;
	bool bg = false;
	int cmd = 1;
	// for (int i = 0; i < len_tokens; i++) {
	// 	printf("argv1[%d] = %s\n", i, tokens[i]);
	// }

	for (int i = 0; i < len_tokens; i++) {
		if (strcmp(tokens[i], ">") == 0) {
			if (i == len_tokens - 1) {
				printf("yash: syntax error near unexpected token 'newline'\n");
				_errno = E_SYN_REDIR;
				break;
			} else if (!isalpha(tokens[i + 1][0]) && !isdigit(tokens[i + 1][0])) {
					printf("syntax error, filename expected\n");
					_errno = E_SYN_REDIR;
					break;
			} else {
				*out = tokens[i + 1];
				cmd = 0;
			}
			// use next token as out1
		} else if (strcmp(tokens[i], "<") == 0) {
			if (i == len_tokens - 1) {
				printf("yash: syntax error near unexpected token 'newline'\n");
				_errno = E_SYN_REDIR;
				break;
			} else if (!isalpha(tokens[i + 1][0]) && !isdigit(tokens[i + 1][0])) {
					printf("syntax error, filename expected\n");
					_errno = E_SYN_REDIR;
					break;
			} else {
				*in = tokens[i + 1];
				cmd = 0;
			}
			// use next token as in1
		} else if (strcmp(tokens[i], "2>") == 0) {
			if (i == len_tokens - 1) {
				printf("yash: syntax error near unexpected token 'newline'\n");
				_errno = E_SYN_REDIR;
				break;
			} else if (!isalpha(tokens[i + 1][0]) && !isdigit(tokens[i + 1][0])) {
					printf("syntax error, filename expected\n");
					_errno = E_SYN_REDIR;
					break;
			} else {
				*err = tokens[i + 1];
				cmd = 0;
			}
			// use next token as stderr
		} else if (strcmp(tokens[i], "|") == 0) {
			if (i == len_tokens - 1) {
				printf("| found unexpectedly\n");
				_errno = E_SYN_PIPE;
				break;
			} else if (len_args1 == 0) {
				printf("expected command before pipe\n");	
				_errno = E_SYN_PIPE;
				break;
			} else {
				out = &out2;
				in = &in2;
				err = &err2;
				cmd = 2;
			}
			// use previous cmd as in1 to next cmd
		} else if (strcmp(tokens[i], "&") == 0) {
			if (i != len_tokens - 1) {
				printf("& found unexpectedly\n");
				_errno = E_SYN_BG;
				break;
			} else {
				bg = true;
				// put program in background
			}
		} else {
			if (cmd == 1) {			// cmd determines which args the token is part of
				// printf("adding to cmd1\n");
				cur_args1[len_args1] = tokens[i];
				len_args1++;
			} else if (cmd == 2){
				// printf("adding to cmd2\n");
				cur_args2[len_args2] = tokens[i];
				len_args2++;
			}
		}
	}
	if (_errno == 0 && len_args1 > 0) {
		cur_args1[len_args1] = NULL;		// append NULL to each for use for execvp later on
		cur_args2[len_args2] = NULL;
		_errno = run_cmds (
			cur_args1, cur_args2,
			len_args1, len_args2,
			out1, in1, err1,
			out2, in2, err2,
			bg
		);
	}

	return _errno;
}

int main() {
	int cpid1, cpid2;
	char in1[2000];
	char * argv[100];

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
  		printf("\ncan't catch SIGINT\n");
	}
	if (signal(SIGCHLD, handler) == SIG_ERR) {
		printf("\ncan't catch SIGCHLD\n");
	}

	while (1) {
		int input_len = get_input(in1);
		int token_len = split(in1, " ", argv);
		// printf("tokens found = %d\n", token_len);
		// for (int j = 0; j < len; j++) {
		// printf("%s\n", argv[j]);
		// }
		if (token_len > 0) {
			parse_tokens_and_run(argv, token_len);
		}
	}
}
