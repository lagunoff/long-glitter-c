#include <malloc.h>
#include <math.h>
#include <string.h>

#include "buff-string.h"

int buff_string_length(struct buff_string *str) {
  struct splice *iter = str->splices;
  int i=str->len;
  for(;iter;) {
    i+=iter->len;
    i-=iter->deleted;
    iter=iter->next;
    if (iter==str->splices) break;
  }
  return i;
}

char buff_string_index(struct buff_string *str, int i) {
  // TODO
}

struct splice *new_splice(int len) {
  int alloc_len = pow(2, ceil(log2(len)));
  printf("len alloc_len: %d %d\n", len, alloc_len);
  struct splice *s = malloc(sizeof(struct splice) + alloc_len);
  memset(s, 0, sizeof(struct splice) + alloc_len);
  s->len = len;
  s->alloc_len = alloc_len;
  return s;
}

void free_buff_string(struct buff_string *str) {
  struct splice *iter = str->splices;
  for(;iter;) {
    free(iter);
    iter=iter->next;
    if (iter==str->splices) break;
  }
}

void buff_string_begin(struct buff_string *str, struct buff_string_iter *iter) {
  iter->is_ended = buff_string_length(str) == 0;
  iter->index = 0;
  iter->str = str;
  iter->iter = str->splices;
}
