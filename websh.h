/**
 * @file websh.h
 * @brief header file for the implementation of websh
 * @author Paul Pröll, 1525669
 * @date 2016-11-15
 */

#ifndef WEBSH_H_
#define WEBSH_H_

static void split(char *str, char **args, char *delimiter); 

static void remove_line_break(char *str);

void child1_execution(int *pipefd, char **args);

void child2_execution(int *pipefd, int line);

static void bail_out(int exitcode, const char *fmt, ...);

static void parse_args(int argc, char** argv);

static void free_resources(void);

#endif
