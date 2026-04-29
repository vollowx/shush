#ifndef SHUSH_BUILTIN_H
#define SHUSH_BUILTIN_H

#include <string.h>

#include "config.h"
#include "info.h"

int shush_cd(shush_info_t *info, char **args);
int shush_help(shush_info_t *info, char **args);
int shush_welcome(shush_info_t *info, char **args);
int shush_exit(shush_info_t *info, char **args);

extern char *builtin_str[];
extern int (*builtin_func[])(shush_info_t *, char **);
size_t shush_num_builtins();

#endif // SHUSH_BUILTIN_H
