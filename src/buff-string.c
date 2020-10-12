#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "buff-string.h"
#include "main.h"

int buff_string_length(struct buff_string *str) {
  return str->len;
}

void buff_string_index(struct buff_string *str, struct buff_string_iter *iter, int i) {
  iter->current = iter->str->bytes + i;
}

struct splice *new_splice(int len) {
  struct splice *s = malloc(sizeof(struct splice));
  memset(s, 0, sizeof(struct splice));
  s->bytes = malloc(len);
  memset(s->bytes, 0, len);
  s->len = len;
  return s;
}

struct splice *new_splice_str(char *str) {
  int len = strlen(str);
  struct splice *result = new_splice(len);
  memcpy(result->bytes, str, len);
  return result;
}

void free_buff_string(struct buff_string *str) {
  struct splice *iter = str->first;
  for(;iter;) {
    struct splice *temp = iter;
    iter=iter->next;
    free(temp);
  }
}

bool buff_string_find(struct buff_string_iter *iter, bool (*p)(char)) {
  int i=0;
  char *end = iter->str->bytes + iter->str->len;

  for(;iter->current < end;iter->current++) {
    if (p(*(iter->current))) goto found;
  }

  found:
  return true;

  eof:
  return false;
}

bool buff_string_find_back(struct buff_string_iter *iter, bool (*p)(char)) {
  int i = 0;

  for(;iter->current >= iter->str->bytes;iter->current--) {
    if (p(*(iter->current))) goto found;
  }

  found:
  return true;

  eof:
  iter->current = iter->str->bytes;
  return false;
}

void buff_string_takewhile(struct buff_string_iter *iter, char *dest, bool (*p)(char)) {
  int j = 0;
  int i = 0;
  char *end = iter->str->bytes + iter->str->len;

  for(;iter->current < end;iter->current++) {
    if (!p(*(iter->current))) goto pred_end;
    dest[j++] = *(iter->current);
  }
  goto eof;

  pred_end:
  dest[j]='\0';
  return;

  eof:
  dest[j]='\0';
  return;
}

void buff_string_begin(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->bytes;
}

bool buff_string_is_begin(struct buff_string_iter *iter) {
  return iter->current == iter->str->bytes;
}

void buff_string_end(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->bytes + str->len;
}

bool buff_string_is_end(struct buff_string_iter *iter) {
  return iter->current == iter->str->bytes + iter->str->len;
}

bool buff_string_move(struct buff_string_iter *iter, int dx) {
  if (dx > 0) {
    int i = 0;
    return buff_string_find(iter, lambda(bool _(char c) {
      return !(i++ < dx);
    }));
  }
  if (dx < 0) {
    int i = 0;
    return buff_string_find_back(iter, lambda(bool _(char c) {
      return !(i++ < abs(dx));
    }));
  }
  return true;
}

int buff_string_offset(struct buff_string_iter *iter) {
  return iter->current - iter->str->bytes;
}

void buff_string_insert(struct buff_string_iter *iter, insert_dir dir, char *str, int deleted, ...) {
  inspect(%d, iter->current - iter->str->bytes);
  inspect(%s, str);
  int insert_offset = iter->current - iter->str->bytes + (dir == INSERT_LEFT ? -deleted : 0);
  int insert_len = strlen(str);
  int new_len = iter->str->len + insert_len - deleted;
  int move_len = iter->str->len - insert_offset - deleted;
  char *new_bytes = iter->str->bytes;

  if (iter->str->len != new_len) {
    new_bytes = realloc(iter->str->bytes, new_len);
  }

  iter->current = new_bytes + insert_offset + (dir == INSERT_LEFT ? insert_len : 0);

  va_list iters;
  va_start(iters, deleted);
  while(1) {
    struct buff_string_iter *it = va_arg(iters, struct buff_string_iter *);
    if (it == NULL) break;
    it->current = new_bytes + (it->current - it->str->bytes);
  }
  va_end(iters);

  char *move_dest = new_bytes + insert_offset + insert_len ;
  char *move_src = new_bytes + insert_offset + deleted;

  inspect(%d, insert_offset);
  inspect(%d, insert_len);
  inspect(%d, deleted);
  inspect(%d, move_len);
  inspect(%lu, new_bytes);
  inspect(%lu, move_dest);
  inspect(%lu, move_src);

  if (move_src != move_dest) memmove(move_dest, move_src, move_len);
  if (insert_len > 0) memcpy(move_dest - insert_len, str, insert_len);
  iter->str->len = new_len;
  iter->str->bytes = new_bytes;
}

void buff_string_forward_word(struct buff_string_iter *iter) {
  buff_string_find(iter, lambda(bool _(char c) { return isalnum(c); }));
  buff_string_find(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

void buff_string_backward_word(struct buff_string_iter *iter) {
  buff_string_find_back(iter, lambda(bool _(char c) { return isalnum(c); }));
  buff_string_find_back(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

void buff_string_unittest() {
  char *s01 = "1234567890abcdefjhijklmnopqrstuvwxyz1234567890";
  char temp[512];
  struct buff_string str01 = {s01, strlen(s01), NULL, NULL};
  struct buff_string_iter iter = BUFF_STRING_BEGIN_INIT(&str01);

  bool found = buff_string_find(&iter, lambda(bool _(char c) {
    return c=='z';
  }));

  assert(found);
  assert(*(iter.current) == 'z');

  buff_string_end(&iter, &str01);
  found = buff_string_find_back(&iter, lambda(bool _(char c) {
     return c=='z';
  }));

  assert(found);
  assert(*(iter.current) == 'z');

  buff_string_begin(&iter, &str01);
  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));
  assert(strcmp(temp, "1234567890")==0);

  struct splice *splice02 = new_splice_str("0000000000");
  splice02->start=0;
  struct buff_string str02 = {s01, strlen(s01), splice02, splice02};

  buff_string_begin(&iter, &str02);

  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));

  printf("temp=%s\n", temp);
  assert(strcmp(temp, "00000000001234567890")==0);
  buff_string_begin(&iter, &str02);
  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
    return true;
  }));
  printf("temp=%s\n", temp);
  assert(strcmp(temp, "00000000001234567890abcdefjhijklmnopqrstuvwxyz1234567890")==0);

  buff_string_index(&str02, &iter, 22);
  printf("iter->next=%p\n", iter.next);
  printf("iter->current=%c\n", *(iter.current));

  assert(*(iter.current) == 'c');

  buff_string_begin(&iter, &str02);
  buff_string_find(&iter, lambda(bool _(char c) {
    return c=='k';
  }));

  inspect("%p", str02.first);
  inspect("%p", str02.last);
  int offset = buff_string_offset(&iter);
  printf("offset=%d\n", offset);
  assert(offset==30);

  free_buff_string(&str01);
  free_buff_string(&str02);
}
