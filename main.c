#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_REALLOC(buffer, type)                                              \
  do {                                                                         \
    buf_size *= 1.5;                                                           \
    buffer = realloc(buffer, buf_size * sizeof(type));                         \
    if (!buffer) {                                                             \
      fprintf(stderr, "reallocation failed");                                  \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define SHUSH_EXECUTABLE_NAME "shush" // TODO: Define in Makefile
#define SHUSH_LINE_BUF_SIZE 1024
#define SHUSH_TOKEN_BUF_SIZE 64
#define SHUSH_TOKEN_DELIM " \t\r\n\a"

char *shush_read_line(void) {
  int buf_size = SHUSH_LINE_BUF_SIZE;
  int pos = 0;
  char *buffer = malloc(sizeof(char) * buf_size);
  int c;
  if (!buffer) {
    fprintf(stderr, "shush: failed to allocate line buffer");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buffer[pos] = '\0';
      return buffer;
    } else {
      buffer[pos] = c;
      ++pos;
    }

    if (pos >= buf_size) {
      BUF_REALLOC(buffer, char);
    }
  }
}

char **shush_parse_line(char *line) {
  int buf_size = SHUSH_TOKEN_BUF_SIZE, pos = 0;
  char **tokens = malloc(sizeof(char *) * buf_size);
  char *token;
  if (!tokens) {
    fprintf(stderr, "%s: failed to allocate token buffer", SHUSH_EXECUTABLE_NAME);
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SHUSH_TOKEN_DELIM);
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

bool shush_should_exit = 0;

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

int shush_cd(char **args);
int shush_help(char **args);
int shush_exit(char **args);
char *builtin_str[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&shush_cd, &shush_help, &shush_exit};

int shush_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int shush_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "shush: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("shush");
    }
  }
  return 0;
}

int shush_help(char **args) {
  int i;
  printf("%s the shell\n", SHUSH_EXECUTABLE_NAME);
  printf("Builtins:\n");

  for (i = 0; i < shush_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 0;
}

int shush_exit(char **args) {
  shush_should_exit = true;
  return 0;
}

int shush_run(char **args) {
  int i;

  if (args[0] == NULL) {
    return 0;
  }

  for (i = 0; i < shush_num_builtins(); ++i) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return shush_execute(args);
}

void shush_loop(void) {
  char *line;
  char **args;
  static int status;

  do {
    printf("%3d:> ", status);
    line = shush_read_line();
    args = shush_parse_line(line);
    status = shush_run(args);

    free(line);
    free(args);
  } while (!shush_should_exit);
}

int main(int argc, char **argv) {
  printf("Welcome to %s, %s\n", "dearPC", "Sailor");
  shush_loop();

  return EXIT_SUCCESS;
}
