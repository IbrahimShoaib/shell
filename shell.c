#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <libgen.h>
#include <ctype.h>

void interruptHandler(int sig_num) {
  // Do nothing on Ctrl-C
  const char* msg = "\n## 3230yash >> ";
  write(STDOUT_FILENO, msg, strlen(msg));
}

int parseCommand(char *command_str, char **arguments) {
  char *saveptr_command;
  char *command = strtok_r(command_str, " ", &saveptr_command);
  int count = 0;

  while (command != NULL) {
    // add to arguments list
    if (count >= 30) {
      printf("3230yash: Too many arguments\n");
      return -1; // error
    }
    
    arguments[count] = command;
    count++;
    command = strtok_r(NULL, " ", &saveptr_command);
  }
  arguments[count] = NULL; // null terminate for execvp
  return count;
}

void execute_single_command(char **arguments) {
  if (arguments == NULL || arguments[0] == NULL) {
    return; // No command to execute
  }

  pid_t pid = fork();
  if (pid < 0) {
    return;
  } else if (pid == 0) {
    signal(SIGINT, SIG_DFL);
    if (execvp(arguments[0], arguments) == -1) {
      fprintf(stderr, "3230yash: '%s': %s\n", arguments[0], strerror(errno));
      exit(1);
    }
  } else {
    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status)) {
      int signum = WTERMSIG(status);
      char *cmd_name = basename(arguments[0]);
      fprintf(stderr, "%s: %s\n", cmd_name, strsignal(signum));
    }
  }
}

void execute_pipeline(char **command_list, int command_count) {
  int num_pipes = command_count - 1;
  int pipes[num_pipes][2];
  pid_t pids[command_count];

  for (int i = 0; i < num_pipes; i++) {
    if (pipe(pipes[i]) < 0) {
      return;
    }
  }

  for (int i = 0; i < command_count; i++) {
    pids[i] = fork();
    if (pids[i] < 0) {
      return;
    }

    if (pids[i] == 0) { // child process
      signal(SIGINT, SIG_DFL);
      
      if (i > 0) {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }
      if (i < num_pipes) {
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      for (int j = 0; j < num_pipes; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      char *arguments[31];
      if (parseCommand(command_list[i], arguments) > 0) {
        execvp(arguments[0], arguments);
        fprintf(stderr, "3230yash: '%s': %s\n", arguments[0], strerror(errno));
      }
      exit(1);
    }
  }

  for (int i = 0; i < num_pipes; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  for (int i = 0; i < command_count; i++) {
    int status;
    waitpid(pids[i], &status, 0);

    if (WIFSIGNALED(status)) {
      int signum = WTERMSIG(status);
      char *cmd_copy = strdup(command_list[i]);
      char * cmd_token = strtok(cmd_copy, " ");
      if (cmd_token) {
        char *cmd_name = basename(cmd_token);
        fprintf(stderr, "%s: %s\n", cmd_name, strsignal(signum));
      }
      free(cmd_copy);
    }
  }
}


struct Snapshot {
  char state; // 3
  int CPUID; // 39
  unsigned long utime; // 14
  unsigned long stime; // 15
  unsigned long vsize; // 23
  unsigned long minflt; // 10
  unsigned long majflt; // 12
};

bool get_snapshot(pid_t pid, struct Snapshot *dest) {
  char path[100];
  snprintf(path, sizeof(path), "/proc/%d/stat", pid); // requires Linux

  FILE *fptr = fopen(path, "r");
  if (fptr == NULL) {
    return false; // process terminated
  }

  // read the needed data
  fscanf(fptr, "%*d %*s %c %*d %*d %*d %*d %*d %*u %lu %*lu %lu %*lu %lu %lu %*ld %*ld %*ld %*ld %*ld %*ld %*llu %lu %*ld %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*lu %*d %d",
         &dest->state,
         &dest->minflt,
         &dest->majflt,
         &dest->utime,
         &dest->stime,
         &dest->vsize,
         &dest->CPUID
  );
  fclose(fptr);
  return true;
}


void execute_watch(char **arguments) {

  if (arguments == NULL || arguments[0] == NULL) {
    return; // no command to execute
  }

  int max_snapshots = 1024;
  struct Snapshot snapshots[max_snapshots];
  int snapshot_count = 0;

  pid_t pid = fork();
  if (pid < 0) {
    return;
  }
  if (pid == 0) {
    signal(SIGINT, SIG_DFL);
    execvp(arguments[0], arguments);
    fprintf(stderr, "3230yash: '%s': %s\n", arguments[0], strerror(errno));
    exit(1);
  } else {
    while (1) {

      if (snapshot_count < max_snapshots) {
        if (get_snapshot(pid, &snapshots[snapshot_count])) {
          snapshot_count++;
        }
      }

      int status;
      pid_t result = waitpid(pid, &status, WNOHANG);

      if (result == pid) {
        break; // child terminated
      }

      usleep(500000);
    }

    long ticks_per_sec = sysconf(_SC_CLK_TCK);

    // process terminated; print snapshots
    // printf("STATE CPUID UTIME STIME VSIZE MINFLT MAJFLT\n");
    printf("%-5s %-6s %-8s %-8s %-12s %-8s %-8s\n", "STATE", "CPUID", "UTIME", "STIME", "VSIZE", "MINFLT", "MAJFLT");

    for (int i = 0; i < snapshot_count; i++) {

      double utime_sec = (double)snapshots[i].utime / ticks_per_sec;
      double stime_sec = (double)snapshots[i].stime / ticks_per_sec;

      printf("%-5c %-6d %-8.2f %-8.2f %-12lu %-8lu %-8lu\n",
             snapshots[i].state,
             snapshots[i].CPUID,
             utime_sec,
             stime_sec,
             snapshots[i].vsize,
             snapshots[i].minflt,
             snapshots[i].majflt
      );
    }
  }
}


int main(int argc, char *argv[]) {
  char input[1024];
  char *command_list[1024]; // tokens array

  signal(SIGINT, interruptHandler); // ignore ctrl-c

  while (1) {

    printf("## 3230yash >> ");
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
      printf("\n");
      break;
    }

    // remove trailing newline
    input[strcspn(input, "\n")] = 0;

    // for empty input
    if (strlen(input) == 0) {
      continue;
    }

    if (input[0] == '|' || input[strlen(input) - 1] == '|') {
      printf("3230yash: Incorrect pipe sequence\n");
      continue;
    }

    if (strstr(input, "||") != NULL) {
      printf("3230yash: should not have two consecutive | without in-between command\n");
      continue;
    }

    if (strcmp(input, "exit") == 0) {
      printf("3230yash: Terminated\n");
      break;
    }

    // if input includes exit but with other args then print message and continue
    if (strncmp(input, "exit", 4) == 0) {
      char *extra = input + 4;
      while (*extra == ' ') extra++; // skip spaces
      if (*extra != '\0') {
        printf("3230yash: \"exit\" with other arguments!!!\n");
        continue;
      }
    }

    char *input_copy = strdup(input);
    char *saveptr_token;
    char *token = strtok_r(input_copy, "|", &saveptr_token); // command including args
    int command_count = 0;
    bool pipe_error = false;

    while (token != NULL) {

      while (*token && isspace(*token)) token++;
      char *end = token + strlen(token) - 1;
      while (end > token && isspace(*end)) end--;
      *(end + 1) = '\0';

      if (strlen(token) == 0) {
        pipe_error = true;
        break;
      }

      command_list[command_count] = token;
      command_count++;
      token = strtok_r(NULL, "|", &saveptr_token);
    }

    if (pipe_error) {
      printf("3230yash: should not have two consecutive | without in-between command\n");
      free(input_copy);
      continue;
    }

    if (command_count == 0) {
      free(input_copy);
      continue;
    }

    char *check_arguments[31];
    char *command_str_copy = strdup(command_list[0]);

    int arg_count = parseCommand(command_str_copy, check_arguments);

    if (arg_count > 0 && strcmp(check_arguments[0], "watch") == 0) {
      if (command_count > 1) {
        printf("3230yash: Cannot watch a pipe sequence\n");
      } else {
        execute_watch(check_arguments + 1);
      }
      free(command_str_copy);
    } else {
      free(command_str_copy);
      if (command_count == 1) {
        char *arguments[31];
        if (parseCommand(command_list[0], arguments) > 0) {
          execute_single_command(arguments);
        }
      } else {
        execute_pipeline(command_list, command_count);
      }
    }

    free(input_copy);
  }
  return 0;
}