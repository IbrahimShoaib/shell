#include "shell.h"

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;
  char input[MAX_CMD_LEN];
  char *command_list[MAX_CMD_LEN]; // tokens array
  char history[MAX_CMD_LEN][MAX_CMD_LEN];
  int history_count = 0;

  signal(SIGINT, interruptHandler); // ignore ctrl-c

  while (1)
  {

    write(STDOUT_FILENO, SHELL_PROMPT, strlen(SHELL_PROMPT));

    if (fgets(input, sizeof(input), stdin) == NULL)
    {
      printf("\n");
      break;
    }

    if (history_count < MAX_CMD_LEN)
    {
      strncpy(history[history_count], input, MAX_CMD_LEN - 1);
      history[history_count][MAX_CMD_LEN - 1] = '\0';
      history_count++;
    }

    // remove trailing newline
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0)
    {
      continue;
    }

    if (input[0] == '|' || input[strlen(input) - 1] == '|')
    {
      printf(SHELL_PREFIX "Incorrect pipe sequence\n");
      continue;
    }

    if (strstr(input, "||") != NULL)
    {
      printf(SHELL_PREFIX "should not have two consecutive | without in-between command\n");
      continue;
    }

    char *input_copy = strdup(input);
    if (input_copy == NULL)
    {
      perror("strdup failed");
      continue;
    }
    char *saveptr_token;
    char *token = strtok_r(input_copy, "|", &saveptr_token); // command including args
    int command_count = 0;
    bool pipe_error = false;

    while (token != NULL)
    {

      while (*token && isspace(*token))
        token++;
      char *end = token + strlen(token) - 1;
      while (end > token && isspace(*end))
        end--;
      *(end + 1) = '\0';

      if (strlen(token) == 0)
      {
        pipe_error = true;
        break;
      }

      command_list[command_count] = token;
      command_count++;
      token = strtok_r(NULL, "|", &saveptr_token);
    }

    if (pipe_error)
    {
      printf(SHELL_PREFIX "Incorrect pipe sequence\n");
      free(input_copy);
      continue;
    }

    if (command_count == 0)
    {
      free(input_copy);
      continue;
    }

    // Check for builtins

    char *first_cmd_copy = strdup(command_list[0]);
    if (first_cmd_copy == NULL)
    {
      perror("strdup failed");
      free(input_copy);
      continue;
    }
    char *check_args[MAX_ARGS];
    int check_count = parseCommand(first_cmd_copy, check_args);

    if (check_count > 0)
    {
      int builtin_status = handle_builtin(check_args, check_count, command_count, history, history_count);
      if (builtin_status == 2)
      { // Exit
        free(first_cmd_copy);
        free(input_copy);
        break;
      }
      if (builtin_status == 1)
      { // built-in handled
        free(first_cmd_copy);
        free(input_copy);
        continue;
      }
    }
    free(first_cmd_copy);

    // Not a builtin, execute normally
    if (command_count == 1)
    {
      char *arguments[MAX_ARGS];
      if (parseCommand(command_list[0], arguments) > 0)
      {
        execute_single_command(arguments);
      }
    }
    else
    {
      execute_pipeline(command_list, command_count);
    }

    free(input_copy);
  }
  return 0;
}
