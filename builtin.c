#include "builtin.h"

#include <stdio.h>
#include <unistd.h>

char *builtin_str[] = {"cd", "help", "welcome", "exit"};
int (*builtin_func[])(shush_info_t *info, char **) = {
    &shush_cd, &shush_help, &shush_welcome, &shush_exit};

size_t shush_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int shush_cd(shush_info_t *info, char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "%s: expected argument to \"cd\"\n", SHUSH_EXECUTABLE_NAME);
  } else {
    if (chdir(args[1]) != 0) {
      perror(SHUSH_EXECUTABLE_NAME);
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
