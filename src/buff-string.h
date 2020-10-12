#pragma once
#include <stdbool.h>

typedef enum {
  MB_FALSE,
  MB_TRUE,
  MB_MORE
} maybe_bool;

typedef enum {
  INSERT_RIGHT,
  INSERT_LEFT,
} insert_dir;

struct splice {
  struct splice *next;
  struct splice *prev;
  int start;
  int deleted;
  int len; // Length of bytes field
  char *bytes;
};

struct buff_string {
  char *bytes;
  int len;
  struct splice *first;
  struct splice *last;
};

struct buff_string_iter {
  char *current; // Possibly NULL
  struct buff_string *str;
  struct splice **next;
};

void free_buff_string(struct buff_string *str);
int buff_string_length(struct buff_string *str);
void buff_string_takewhile(struct buff_string_iter *iter, char *out, bool (*p)(char));
void buff_string_index(struct buff_string *str, struct buff_string_iter *iter, int i);
void buff_string_begin(struct buff_string_iter *iter, struct buff_string *str);
void buff_string_end(struct buff_string_iter *iter, struct buff_string *str);
bool buff_string_find(struct buff_string_iter *iter, bool (*p)(char));
bool buff_string_find_back(struct buff_string_iter *iter, bool (*p)(char));
bool buff_string_move(struct buff_string_iter *iter, int dx);
int buff_string_offset(struct buff_string_iter *iter);
void buff_string_insert(struct buff_string_iter *iter, insert_dir dir, char *str, int deleted, ...);
bool buff_string_is_begin(struct buff_string_iter *iter);
bool buff_string_is_end(struct buff_string_iter *iter);
void inspect_splices_deep (struct buff_string *str);
void inspect_splices_deep_2 (struct buff_string_iter *iter);

// Deallocate memory with `free`
struct splice *new_splice(int len);
struct splice *new_splice_str(char *str);

#define BUFF_STRING_BEGIN_INIT(str) {(str)->first && (str)->first->start==0 ? (str)->first->bytes : (str)->bytes, (str), &((str)->first)}
