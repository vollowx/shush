#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

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
#define SHUSH_PROMPT_BUF_SIZE 1024
#define SHUSH_TOKEN_BUF_SIZE 64
#define SHUSH_TOKEN_DELIM " \t\r\n\a"

typedef struct {
  char *hostname;
  char *username;

  bool is_root;
  int status;
  bool should_exit;
} shush_info_t;

shush_info_t *shush_info_alloc() {
  shush_info_t *info = malloc(sizeof(shush_info_t));
  if (!info)
    goto error_alloc;

  info->hostname = malloc(sizeof(char) * 64);
  if (!info->hostname)
    goto error_suballoc;

  info->is_root = false;
  info->status = 0;
  info->should_exit = false;

  return info;

error_suballoc:
  free(info->hostname);
error_alloc:
  free(info);
  return NULL;
}
void shush_info_get(shush_info_t *info) {
  if (gethostname(info->hostname, 64) != 0)
    perror("gethostname");
  if ((info->username = getenv("USER")) == NULL)
    fprintf(stderr, "%s: failed to get username", SHUSH_EXECUTABLE_NAME);
  if (geteuid() == 0)
    info->is_root = true;
}
void shush_info_free(shush_info_t *info) {
  free(info->hostname);
  free(info);
}

char *shush_read_line(shush_info_t *shush_info) {
  char *prompt_buffer = malloc(sizeof(char) * SHUSH_PROMPT_BUF_SIZE);
  sprintf(prompt_buffer, "(%d) %c ", shush_info->status, shush_info->is_root ? '#' : '$');

  char *buffer = readline(prompt_buffer);

  free(prompt_buffer);

  return buffer;
}

char **shush_parse_line(char *line) {
  int buf_size = SHUSH_TOKEN_BUF_SIZE, pos = 0;
  char **tokens = malloc(sizeof(char *) * buf_size);
  char *token;
  if (!tokens) {
    fprintf(stderr, "%s: failed to allocate token buffer",
            SHUSH_EXECUTABLE_NAME);
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

int shush_cd(shush_info_t *info, char **args);
int shush_help(shush_info_t *info, char **args);
int shush_welcome(shush_info_t *info, char **args);
int shush_exit(shush_info_t *info, char **args);
char *builtin_str[] = {"cd", "help", "welcome", "exit"};

int (*builtin_func[])(shush_info_t *info, char **) = {
    &shush_cd, &shush_help, &shush_welcome, &shush_exit};

int shush_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int shush_cd(shush_info_t *info, char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "%s: expected argument to \"cd\"\n", SHUSH_EXECUTABLE_NAME);
  } else {
    if (chdir(args[1]) != 0) {
      perror("shush");
    }
  }
  return 0;
}

int shush_help(shush_info_t *info, char **args) {
  int i;
  printf("%s the shell\n", SHUSH_EXECUTABLE_NAME);
  printf("Builtins:\n");

  for (i = 0; i < shush_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 0;
}

int shush_welcome(shush_info_t *info, char **args) {
  printf("Welcome to %s, %s\n", info->hostname, info->username);
  return 0;
}

int shush_exit(shush_info_t *info, char **args) {
  info->should_exit = true;
  return 0;
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
  char **args;

  do {
    line = shush_read_line(info);
    args = shush_parse_line(line);
    info->status = shush_run(info, args);

    free(line);
    free(args);
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
