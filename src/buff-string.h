#pragma once
#include <stdbool.h>

struct splice {
  struct splice *next;
  struct splice *prev;
  int start;
  int deleted;
  int len; // Length of bytes field
  int alloc_len; // Allocated length of bytes
  char bytes[0];
};

struct buff_string {
  char *bytes;
  int len;
  struct splice *splices;
};

struct buff_string_iter {
  bool is_ended;
  char value;
  int index;
  struct buff_string *str;
  struct splice *iter;
};

void free_buff_string(struct buff_string *str);
int buff_string_length(struct buff_string *str);
char buff_string_index(struct buff_string *str, int i);
void buff_string_begin(struct buff_string *str, struct buff_string_iter *iter);

// Deallocate memory with `free`
struct splice *new_splice(int len);

#define BUFF_STRING_BEGIN(str) {buff_string_length((str)) == 0, 0, 0, (str), (str)->splices}
