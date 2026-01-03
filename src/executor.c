#include "shell.h"

void execute_single_command(char **arguments)
{
  if (arguments == NULL || arguments[0] == NULL)
  {
    return; // No command to execute
  }

  pid_t pid = fork();
  if (pid < 0)
  {
    perror("fork");
    return;
  }
  else if (pid == 0)
  {
    signal(SIGINT, SIG_DFL);
    if (execvp(arguments[0], arguments) == -1)
    {
      fprintf(stderr, "3230yash: '%s': %s\n", arguments[0], strerror(errno));
      exit(1);
    }
  }
  else
  {
    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status))
    {
      int signum = WTERMSIG(status);
      char *cmd_name = basename(arguments[0]);
      fprintf(stderr, "%s: %s\n", cmd_name, strsignal(signum));
    }
  }
}

void execute_pipeline(char **command_list, int command_count)
{
  int num_pipes = command_count - 1;
  int pipes[num_pipes][2];
  pid_t pids[command_count];

  for (int i = 0; i < num_pipes; i++)
  {
    if (pipe(pipes[i]) < 0)
    {
      perror("pipe");
      // Cleanup previously opened pipes
      for (int j = 0; j < i; j++)
      {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      return;
    }
  }

  for (int i = 0; i < command_count; i++)
  {
    pids[i] = fork();
    if (pids[i] < 0)
    {
      perror("fork");
      // Cleanup
      for (int j = 0; j < num_pipes; j++)
      {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }
      return;
    }

    if (pids[i] == 0)
    {
      signal(SIGINT, SIG_DFL);

      if (i > 0)
      {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }
      if (i < num_pipes)
      {
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      for (int j = 0; j < num_pipes; j++)
      {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      char *arguments[MAX_ARGS];
      if (parseCommand(command_list[i], arguments) > 0)
      {
        execvp(arguments[0], arguments);
        fprintf(stderr, "3230yash: '%s': %s\n", arguments[0], strerror(errno));
      }
      exit(1);
    }
  }

  for (int i = 0; i < num_pipes; i++)
  {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  for (int i = 0; i < command_count; i++)
  {
    int status;
    waitpid(pids[i], &status, 0);

    if (WIFSIGNALED(status))
    {
      int signum = WTERMSIG(status);
      char *cmd_copy = strdup(command_list[i]);
      if (cmd_copy != NULL)
      {
        char *cmd_token = strtok(cmd_copy, " ");
        if (cmd_token)
        {
          char *cmd_name = basename(cmd_token);
          fprintf(stderr, "%s: %s\n", cmd_name, strsignal(signum));
        }
        free(cmd_copy);
      }
    }
  }
}
