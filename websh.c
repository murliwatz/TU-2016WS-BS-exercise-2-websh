/**
 * @brief main c file for the implementation of websh
 * @author Paul Pröll, 1525669
 * @date 2016-10-08
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include "websh.h"

bool html_support = false;
bool print_headers = false;
bool replacing = false;
char word[20];
char tag[10];
char lines[10][60];

int pipefd[2]; // pipes for inter communication
FILE *pp; // file descriptor for reading lines from pipe

static char* progname;

char *ptr;

/* === Implementations === */

int main(int argc, char** argv) {

	memset(word, '\0', sizeof(word));
	memset(tag, '\0', sizeof(tag));
	memset(lines, '\0', sizeof(lines));

	pipe(pipefd);

	parse_args(argc, argv);

	int line = 0;
	while(fgets (lines[line], sizeof(lines[line]), stdin) !=NULL) {
		line++;
	}

	line = 0;
	while(strlen(lines[line]) > 0) 
	{
		remove_line_break((char*)&lines[line]);

		char *args[10];
		char *delimiter = " ";
		split(lines[line], args, delimiter); 

		int pid, pid2;
		switch( pid=fork() ) {
			case -1:   /* Fehler bei fork() */ 
				bail_out(EXIT_FAILURE, "fork error");
				break;
			case  0:   /* Hier befinden Sie sich im Kindprozess   */
				child1_execution((int*)&pipefd, args);
	     		break;
	  		default:   /* Hier befinden Sie sich im Elternprozess */ 
				switch( pid2=fork() ) {
					case -1:   /* Fehler bei fork() */ 
						bail_out(EXIT_FAILURE, "fork error");
						break;
					case  0:   /* Hier befinden Sie sich im Kindprozess   */
						child2_execution((int*)&pipefd, line);
	     				break;
				}
	    		break;
		}

		line++;
	}

	exit(EXIT_SUCCESS);	
}

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
				ptr = strchr(optarg, ':');
				if(ptr != NULL) {
				   int index = ptr - optarg;
				   strncpy(word, optarg, index);
				   strcpy(tag, optarg + index + 1);
				} else {
					bail_out(EXIT_FAILURE, "Usage: websh [-e] [-h] [-s WORD:TAG]");	
				}
				break;
			default:
				bail_out(EXIT_FAILURE, "Usage: websh [-e] [-h] [-s WORD:TAG]");		
		}
	}
}

void remove_line_break(char *str) {
	char *pos;
	if ((pos=strchr(str, '\n')) != NULL)
 	*pos = '\0';
}

void split(char *str, char **args, char *delimiter) {
	ptr = strtok(str, delimiter);
	int c = 0;
	while(ptr != NULL) {
		args[c] = ptr;
		c++;
	 	ptr = strtok(NULL, delimiter);
	}
	args[c] = 0;
}

void child1_execution(int *pipefd, char **args) {
	close(pipefd[0]);

	dup2(pipefd[1], 1);  // send stdout to the pipe
	dup2(pipefd[1], 2);  // send stderr to the pipe
	    		
	execvp(args[0], &args[0]);

	free_resources();
	exit(EXIT_SUCCESS);
}

void child2_execution(int *pipefd, int line){
	close(pipefd[1]);
	char buffer[1024];

	if(html_support == true && line == 0) {
		fprintf(stdout, "<html><head></head><body>\n");		
	}

	pp = fdopen(pipefd[0], "r");
	while (fgets(buffer, sizeof(buffer), pp) != 0)
	{
		remove_line_break((char*)&buffer);

		if(print_headers == true) {
			fprintf(stdout, "<h1>%s</h1>\n", lines[line]);
		}
		if(replacing == true && strstr(buffer, word) != NULL) {
			fprintf(stdout, "<%s>%s</%s><br />\n", tag, buffer, tag);
		} else {
			fprintf(stdout, "%s<br />\n", buffer);
		}
	}
	fclose(pp);

  	if(html_support == true && strlen(lines[line + 1]) == 0) {
		fprintf(stdout, "</body></html>");		
  	}

	fflush(stdout);

	free_resources();
	exit(EXIT_SUCCESS);
}

static void free_resources(void) {
	if(pipefd[0] > 0) close(pipefd[0]);
	if(pipefd[1] > 0) close(pipefd[1]);
	if(pp != NULL) fclose(pp);
}
