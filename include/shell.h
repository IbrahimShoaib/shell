#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>
#include <ctype.h>
#include <signal.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define PATH_MAX_LEN 256
#define MAX_SNAPSHOTS 1024
#define SHELL_NAME "mosh"
#define SHELL_PROMPT "## " SHELL_NAME " >> "
#define SHELL_PREFIX SHELL_NAME ": "

typedef struct
{
  char state;           // 3
  int CPUID;            // 39
  unsigned long utime;  // 14
  unsigned long stime;  // 15
  unsigned long vsize;  // 23
  unsigned long minflt; // 10
  unsigned long majflt; // 12
} Snapshot;

void interruptHandler(int sig_num);
int parseCommand(char *command_str, char **arguments);
void execute_single_command(char **arguments);
void execute_pipeline(char **command_list, int command_count);
bool get_snapshot(pid_t pid, Snapshot *dest);
void execute_watch(char **arguments);
int handle_builtin(char **args, int args_count, int command_count, char history[][MAX_CMD_LEN], int history_count); // args_count includes command name

#endif