#include <stddef.h>
#include "utils.h"

void noop() {}

char *strchr_last(char *str, int c) {
  char *result = NULL;
  for(int i = 0; str[i]; i++)
    if (str[i] == c) result = str + i;
  return result;
}

char *basename(char *str) {
  char *last_slash = strchr_last(str, '/');
  return last_slash ? last_slash + 1 : str;
}

char *extension(char *str) {
  char *last_dot = strchr_last(str, '.');
  return last_dot ? last_dot + 1 : NULL;
}
