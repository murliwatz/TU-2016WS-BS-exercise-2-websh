/**
 * @file websh.c
 * @brief main c file for the implementation of websh
 * @author Paul Pröll, 1525669
 * @date 2016-11-15
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/wait.h>
#include "websh.h"

/** true if option -e is on, otherwise false */
bool html_support = false; 

/** true if option -h is on, otherwise false */
bool print_headers = false;

/** true if argument -s is set, otherwise false */ 
bool replacing = false;

/** first part of -s WORD:TAG */
char *word;

/** second part of -s WORD:TAG */ 
char *tag; 

/** used to read all commands that should be executed */
char **lines; 

/** buffer for child_execution2 */
char *buffer;

/** pipes for intercommunication */
int pipefd[2]; 

/** file descriptor for reading lines from pipe */
FILE *pp; 

/** name of this program */
static char* progname; 

/**
 * @brief entry point where the program is executed
 */
int main(int argc, char** argv) {

	pipe(pipefd);

	parse_args(argc, argv);

	char *line;
	int line_count = 0;
	if((line = (char *) malloc(BUFFER_SIZE)) == NULL) {
		bail_out(EXIT_FAILURE, "allocation error");
	}
	while(fgets(line, BUFFER_SIZE, stdin) != NULL) {
		line_count++;
		if(lines == NULL) {
			lines = malloc(line_count * sizeof(char *));
		} else {
			lines = realloc(lines, line_count * sizeof(char *));
		}
		lines[line_count - 1] = line;
		if((line = (char *) malloc(BUFFER_SIZE)) == NULL) {
			bail_out(EXIT_FAILURE, "allocation error");
		}
	}

	lines = realloc(lines, (line_count + 1) * sizeof(char *));
	lines[line_count] = NULL;

	for(int i = 0; i < line_count; i++) 
	{
		remove_line_break(lines[i]);

		char **args = NULL;
		char *delimiter = " ";
		args = split(lines[i], delimiter); 

		int pid, pid2;
		switch(pid=fork()) {
			case -1:   /* Fehler bei fork() */ 
				bail_out(EXIT_FAILURE, "fork error");
				break;
			case  0:   /* Hier befinden Sie sich im Kindprozess   */
				child1_execution((int*)&pipefd, args);
	     		break;
	  		default:   /* Hier befinden Sie sich im Elternprozess */ 
				switch(pid2 = fork()) {
					case -1:   /* Fehler bei fork() */ 
						bail_out(EXIT_FAILURE, "fork error");
						break;
					case  0:   /* Hier befinden Sie sich im Kindprozess   */
						child2_execution((int*)&pipefd, i);
	     				break;
				}
	    		break;
		}

		int status = 0;
		int pid_w = wait(&status);
		if (pid_w != pid && pid_w != pid2) {
        	bail_out(EXIT_FAILURE, "wait()");
      	}

	}

	exit(EXIT_SUCCESS);	
}

/**
 * @brief Prints fmt on stderr, free resources and closes program with exitcode
 * @param exitcode Exitcode that should be returned for termination of process
 * @param fmt String to print out
 * @param ...
 */
static void bail_out(int exitcode, const char *fmt, ...)
{
    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (errno != 0) {
        (void) fprintf(stderr, ": %s", strerror(errno));
    }
    (void) fprintf(stderr, "\n");

    free_resources();
    exit(exitcode);
}

/**
 * @brief Parses the program arguments
 * @param argc Count of arguments
 * @param argv Array of arguments
 */
static void parse_args(int argc, char** argv) {
	int opt;

	if(argc > 0) {
		progname = argv[0];
	}
	if(argc > 5) {
		bail_out(EXIT_FAILURE, "Usage: websh [-e] [-h] [-s WORD:TAG]");
	}
	while((opt = getopt(argc, argv, "ehs:")) != -1) {
		switch(opt) {
			case 'e':
				html_support = true;
				break;
			case 'h':
				print_headers = true;
				break;
			case 's':
				replacing = true;
				char *ptr;
				ptr = strchr(optarg, ':');
				if(ptr != NULL) {
				   int index = ptr - optarg;
				   if((word = malloc(index)) == NULL) {
				   		bail_out(EXIT_FAILURE, "Allocating memory for word");
				   }
				   int optarg_length = strlen(optarg);
				   if((tag = malloc(optarg_length)) == NULL) {
				   		bail_out(EXIT_FAILURE, "Allocating memory for tag");
				   }
				   if(strncpy(word, optarg, index) == NULL) {
				   		bail_out(EXIT_FAILURE, "copying word");
				   }
				   if(strncpy(tag, optarg + index + 1, index + optarg_length) == NULL) {
				   		bail_out(EXIT_FAILURE, "copying tag");
				   }
				} else {
					bail_out(EXIT_FAILURE, "Usage: websh [-e] [-h] [-s WORD:TAG]");	
				}
				break;
			default:
				bail_out(EXIT_FAILURE, "Usage: websh [-e] [-h] [-s WORD:TAG]");		
		}
	}
}

/**
 * @brief Removes line breaks could be on the end of a string
 * @param str String that should be changed
 */
void remove_line_break(char *str) {
	char *pos;
	if ((pos=strchr(str, '\n')) != NULL){
 		*pos = '\0';
 	}
}

/**
 * @brief Splits a String to an array of Strings where a delimiter is
 * @param str String that should be splitted
 * @param args Array where the Strings should be written to
 * @param delimiter One or more delimiters
 */
char **split(char *str, char *delimiter) {
	char **args = NULL;
	char *ptr;
	ptr = strtok(str, delimiter);
	int c = 0;
	while(ptr != NULL) {
		if(args == NULL) {
			args = malloc((c + 1) * sizeof(char *));
		} else {
			args = realloc(args, (c + 1) * sizeof(char *));
		}
		if(args == NULL) {
			bail_out(EXIT_FAILURE, "allocation error");
		}
		args[c] = ptr;
		c++;
	 	ptr = strtok(NULL, delimiter);
	}

	if((args = realloc(args, (c + 1) * sizeof(char *))) == NULL) {
		bail_out(EXIT_FAILURE, "allocation error");
	}
	args[c] = NULL;

	return args;
}

/**
 * @brief The code for child procress 1 (execution of a process)
 * @param pipefd Pipes for intercommunication
 * @param args Process with arguments that should be executed
 */
void child1_execution(int *pipefd, char **args) {
	
	if(close(pipefd[0]) == -1) {
		bail_out(EXIT_FAILURE, "closing read pipe");
	}

	// send stdout to the pipe
	if(dup2(pipefd[1], 1) == -1) {
		bail_out(EXIT_FAILURE, "stdout to pipe");
	}  

	// send stderr to the pipe
	if(dup2(pipefd[1], 2) == -1) {
		bail_out(EXIT_FAILURE, "stderr to pipe");
	}    		

	if(execvp(args[0], args) == -1) {
		bail_out(EXIT_FAILURE, "exec error");
	}

	free_resources();
	exit(EXIT_SUCCESS);
}

/**
 * @brief The code for child procress 2 (output generation)
 * @param pipefd Pipes for intercommunication
 * @param line Line number (index for array 'Lines')
 */
void child2_execution(int *pipefd, int line){
	
	if(close(pipefd[1]) == -1) {
		bail_out(EXIT_FAILURE, "closing write pipe");
	}

	if(html_support == true && line == 0) {
		(void) fprintf(stdout, "<html><head></head><body>\n");		
	}

	if(print_headers == true) {
		(void) fprintf(stdout, "<h1>%s</h1>\n", lines[line]);
	}

	if((buffer = (char *) malloc(BUFFER_SIZE)) == NULL) {
		bail_out(EXIT_FAILURE, "allocation error");
	}
	if((pp = fdopen(pipefd[0], "r")) == NULL) {
		bail_out(EXIT_FAILURE, "fdopen failed");
	}
	while (fgets(buffer, BUFFER_SIZE, pp) != 0)
	{
		remove_line_break(buffer);

		if(replacing == true && strstr(buffer, word) != NULL) {
			(void) fprintf(stdout, "<%s>%s</%s><br />\n", tag, buffer, tag);
		} else {
			(void) fprintf(stdout, "%s<br />\n", buffer);
		}

		(void) memset(buffer, '\0', BUFFER_SIZE);
	}

  	if(html_support == true && lines[line + 1] == NULL) {
		(void) fprintf(stdout, "</body></html>");		
  	}

	(void) fflush(stdout);

	free_resources();
	exit(EXIT_SUCCESS);
}

/**
 *  @brief Free allocated resources
 */
static void free_resources(void) {
	if(pipefd[0] > 0) close(pipefd[0]);
	if(pipefd[1] > 0) close(pipefd[1]);
	if(pp != NULL) {
		(void) fclose(pp);
	}
	free(word);
	free(tag);
	int i = 0;
	while(lines[i] != NULL) {
		free(lines[i++]);
	}
	free(lines);
	free(buffer);
}
