#include "shell.h"

int parseCommand(char *command_str, char **arguments)
{
  char *saveptr_command;
  char *command = strtok_r(command_str, " ", &saveptr_command);
  int count = 0;

  while (command != NULL)
  {
    if (count >= MAX_ARGS - 1)
    {
      printf("3230yash: Too many arguments\n");
      return -1;
    }

    arguments[count] = command;
    count++;
    command = strtok_r(NULL, " ", &saveptr_command);
  }
  arguments[count] = NULL;
  return count;
}
