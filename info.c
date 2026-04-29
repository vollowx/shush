#include "info.h"

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
