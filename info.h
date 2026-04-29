#ifndef SHUSH_INFO_H
#define SHUSH_INFO_H

#include <stdbool.h>

typedef struct {
  char *hostname;
  char *username;

  bool is_root;
  int status;
  bool should_exit;
} shush_info_t;

shush_info_t *shush_info_alloc();
void shush_info_get(shush_info_t *info);
void shush_info_free(shush_info_t *info);

#endif // SHUSH_INFO_H
