/**
 * @brief header file for the implementation of websh
 * @author Paul Pröll, 1525669
 * @date 2016-10-08
*/

#ifndef WEBSH_H_
#define WEBSH_H_

/* === Constants === */

/* === Prototypes === */

void split(char *str, char **args, char *delimiter);

void remove_line_break(char *str);

void child1_execution(int *pipefd, char **args);

void child2_execution(int *pipefd, int line);


/*
 * @brief Prints fmt on stderr, free resources and closes program with exitcode
 * @param exitcode Exitcode that should be returned for termination of process
 * @param fmt String to print out
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief Parses the program arguments
 * @param argc Count of arguments
 * @param argv Array of arguments
 */
 static void parse_args(int argc, char** argv);

/**
 *  @brief Free allocated resources
 */
 static void free_resources(void);

#endif
