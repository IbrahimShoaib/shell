#include "shell.h"

void execute_watch(char **arguments)
{
#ifdef __linux__
  if (arguments == NULL || arguments[0] == NULL)
  {
    return; // no command to execute
  }

  Snapshot snapshots[MAX_SNAPSHOTS];
  int snapshot_count = 0;

  pid_t pid = fork();
  if (pid < 0)
  {
    perror("fork");
    return;
  }
  if (pid == 0)
  {
    signal(SIGINT, SIG_DFL);
    execvp(arguments[0], arguments);
    fprintf(stderr, "3230yash: '%s': %s\n", arguments[0], strerror(errno));
    exit(1);
  }
  else
  {
    while (1)
    {

      if (snapshot_count < MAX_SNAPSHOTS)
      {
        if (get_snapshot(pid, &snapshots[snapshot_count]))
        {
          snapshot_count++;
        }
      }

      int status;
      pid_t result = waitpid(pid, &status, WNOHANG);

      if (result == pid)
      {
        break; // child terminated
      }

      usleep(500000);
    }

    long ticks_per_sec = sysconf(_SC_CLK_TCK);

    printf("%-5s %-6s %-8s %-8s %-12s %-8s %-8s\n", "STATE", "CPUID", "UTIME", "STIME", "VSIZE", "MINFLT", "MAJFLT");

    for (int i = 0; i < snapshot_count; i++)
    {

      double utime_sec = (double)snapshots[i].utime / ticks_per_sec;
      double stime_sec = (double)snapshots[i].stime / ticks_per_sec;

      printf("%-5c %-6d %-8.2f %-8.2f %-12lu %-8lu %-8lu\n",
             snapshots[i].state,
             snapshots[i].CPUID,
             utime_sec,
             stime_sec,
             snapshots[i].vsize,
             snapshots[i].minflt,
             snapshots[i].majflt);
    }
  }
#else
  (void)arguments;
  printf("3230yash: watch command is only supported on Linux\n");
#endif
}

int handle_builtin(char **args, int args_count, int command_count, char history[][MAX_CMD_LEN], int history_count)
{
  if (args[0] == NULL)
    return 0;

  if (strcmp(args[0], "exit") == 0)
  {
    if (args[1] != NULL)
    {
      printf("3230yash: \"exit\" with other arguments!!!\n");
      return 1; // Continue
    }
    printf("3230yash: Terminated\n");
    return 2; // Exit
  }

  if (strcmp(args[0], "watch") == 0)
  {
    if (command_count > 1)
    {
      printf("3230yash: Cannot watch a pipe sequence\n");
    }
    else
    {
      execute_watch(args + 1);
    }
    return 1;
  }

  if (strcmp(args[0], "cd") == 0)
  {
    if (args_count == 2)
    {
      printf("3230yash: Changing directory to %s\n", args[1]);
      if (chdir(args[1]) != 0)
      {
        perror("chdir failed");
      }
      else
      {
        printf("Directory changed successfully\n");
      }
    }
    else
    {
      perror("cd: expected one argument");
    }
    return 1;
  }

  if (strcmp(args[0], "history") == 0)
  {
    for (int i = 0; i < history_count; i++)
    {
      printf("%d %s\n", i + 1, history[i]);
    }
    return 1;
  }

  if (strcmp(args[0], "help") == 0)
  {
    printf("3230yash: Supported built-in commands:\n");
    printf("  exit              : Exit the shell\n");
    printf("  cd <directory>    : Change the current directory to <directory>\n");
    printf("  history           : Display the list of previously executed commands\n");
    printf("  watch <command>   : Execute <command> and monitor its resource usage\n");
    printf("  help              : Display this help message\n");
    return 1;
  }

  return 0; // Not a builtin
}
