#include <stddef.h>
#include <string.h>
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

int dirname(char *str, char *out) {
  int last_slash = 0;
  int len = strlen(str);
  for(int i = 0; i < len - 1; i++)
    if (str[i] == '/') last_slash = i;
  int new_len = last_slash == 0 ? len : last_slash;
  strncpy(out, str, new_len);
  return new_len;
}
