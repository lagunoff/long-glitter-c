#pragma once
#include <stdbool.h>

typedef enum {
  BSD_RIGHT,
  BSD_LEFT,
} buff_string_dir;

typedef enum {
  BSI_STATE_ONE,
  BSI_STATE_TWO,
  BSI_STATE_THREE,
  BSI_STATE_FOUR,
} buff_string_iter_state;

typedef enum {
  BSLIP_DO,
  BSLIP_DONT,
} buff_string_last_increment_policy;

struct splice {
  struct splice *next;
  struct splice *prev;
  int start;
  int deleted;
  int len; // Length of bytes field
  char *bytes;
};

struct buff_string {
  struct splice *first;
  struct splice *last;
  char *bytes;
  int len;
};

struct buff_string_iter {
  struct splice *next;
  unsigned int offset;
  bool in_splice;
  struct buff_string *str;
};

void free_buff_string(struct buff_string *str);
int buff_string_length(struct buff_string *str);
bool buff_string_takewhile(struct buff_string_iter *iter, char *out, bool (*p)(char));
void buff_string_index(struct buff_string *str, struct buff_string_iter *iter, int i);
void buff_string_begin(struct buff_string_iter *iter, struct buff_string *str);
void buff_string_end(struct buff_string_iter *iter, struct buff_string *str);
bool buff_string_find(struct buff_string_iter *iter, bool (*p)(char));
bool buff_string_find_back(struct buff_string_iter *iter, bool (*p)(char));
bool buff_string_move(struct buff_string_iter *iter, int dx);
int buff_string_offset(struct buff_string_iter *iter);
void buff_string_insert(struct buff_string_iter *iter, buff_string_dir dir, char *str, int deleted, ...);
void buff_string_forward_word(struct buff_string_iter *iter);
void buff_string_backward_word(struct buff_string_iter *iter);

bool buff_string_iterate(
  struct buff_string_iter *iter,
  buff_string_dir dir,
  buff_string_last_increment_policy last_inc,
  bool (*p)(char)
);

// Deallocate memory with `free`
struct splice *new_splice(int len);
struct splice *new_splice_str(char *str);
