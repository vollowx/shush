#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "builtin.h"
#include "config.h"
#include "info.h"

#define BUF_REALLOC(buffer, type)                                              \
  do {                                                                         \
    buf_size *= 1.5;                                                           \
    buffer = realloc(buffer, buf_size * sizeof(type));                         \
    if (!buffer) {                                                             \
      fprintf(stderr, "reallocation failed");                                  \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

char *shush_read_line(shush_info_t *shush_info) {
  char *prompt_buffer = malloc(sizeof(char) * SHUSH_PROMPT_BUFSIZE);
  sprintf(prompt_buffer, "(%d) %c ", shush_info->status,
          shush_info->is_root ? '#' : '$');

  char *buffer = readline(prompt_buffer);

  free(prompt_buffer);

  return buffer;
}

char **shush_split_pipes(char *line) {
  int buf_size = SHUSH_PIPE_BUFSIZE, pos = 0;
  char **commands = malloc(sizeof(char *) * buf_size);
  char *command;
  if (!commands) {
    fprintf(stderr, "%s: failed to allocate token buffer",
            SHUSH_EXECUTABLE_NAME);
    exit(EXIT_FAILURE);
  }

  command = strtok(line, SHUSH_PIPE_DELIM);
  while (command != NULL) {
    commands[pos] = command;
    ++pos;

    if (pos >= buf_size) {
      BUF_REALLOC(commands, char *);
    }
    command = strtok(NULL, SHUSH_PIPE_DELIM);
  }
  commands[pos] = NULL;
  return commands;
}

char **shush_parse_command(char *command) {
  int buf_size = SHUSH_TOKEN_BUFSIZE, pos = 0;
  char **tokens = malloc(sizeof(char *) * buf_size);
  char *token;
  if (!tokens) {
    fprintf(stderr, "%s: failed to allocate token buffer",
            SHUSH_EXECUTABLE_NAME);
    exit(EXIT_FAILURE);
  }

  token = strtok(command, SHUSH_TOKEN_DELIM);
  while (token != NULL) {
    tokens[pos] = token;
    ++pos;

    if (pos >= buf_size) {
      BUF_REALLOC(tokens, char *);
    }

    token = strtok(NULL, SHUSH_TOKEN_DELIM);
  }
  tokens[pos] = NULL;

  return tokens;
}

int shush_execute(char **args) {
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Is child process
    if (execvp(args[0], args) == -1) {
      perror("shush");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("shush");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return status;
}

int shush_run_pipeline(char **commands) {
  int i = 0;
  int fd_input = 0, fd[2]; // fd[0] for reading, fd[1] for writing

  pid_t pid;
  int status = 0;

  while (commands[i] != NULL) {
    pipe(fd);
    pid = fork();

    if (pid == 0) {
      // Is child process
      if (fd_input !=
          0) { // If isn't the first command, read from the prev pipe
        dup2(fd_input, STDIN_FILENO);
        close(fd_input);
      }
      if (commands[i + 1] != NULL) { // If isn't the last, write to the current
        dup2(fd[1], STDOUT_FILENO);
      }
      close(fd[0]);
      close(fd[1]);

      char **args = shush_parse_command(commands[i]);

      if (args[0] == NULL)
        exit(EXIT_SUCCESS);

      if (execvp(args[0], args) == -1) {
        perror(SHUSH_EXECUTABLE_NAME " pipeline execution");
      }
      exit(EXIT_FAILURE);
    } else if (pid < 0) {
      perror(SHUSH_EXECUTABLE_NAME " pipeline forking");
    } else {
      // Is parent process
      if (fd_input != 0)
        close(fd_input);
      close(fd[1]);
      fd_input = fd[0];
      ++i;
    }
  }

  if (fd_input != 0)
    close(fd_input);
  while (wait(&status) > 0)
    ;

  return status;
}

int shush_run(shush_info_t *info, char **args) {
  int i;

  if (args[0] == NULL) {
    return 0;
  }

  for (i = 0; i < shush_num_builtins(); ++i) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(info, args);
    }
  }

  return shush_execute(args);
}

void shush_loop(shush_info_t *info) {
  char *line;
  char **commands;

  do {
    line = shush_read_line(info);
    commands = shush_split_pipes(line);

    if (commands[0] != NULL && commands[1] == NULL) {
      char **args = shush_parse_command(commands[0]);
      info->status = shush_run(info, args);
      free(args);
    } else {
      info->status = shush_run_pipeline(commands);
    }

    free(commands);
    free(line);
  } while (!info->should_exit);
}

int main(int argc, char **argv) {
  rl_initialize();

  shush_info_t *info = shush_info_alloc();
  shush_info_get(info);

  shush_welcome(info, NULL);
  shush_loop(info);

  shush_info_free(info);

  return EXIT_SUCCESS;
}
