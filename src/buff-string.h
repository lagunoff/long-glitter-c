#pragma once
#include <stdbool.h>

typedef enum {
  BS_RIGHT,
  BS_LEFT,
} bs_direction_t;

typedef enum {
  BS_STATE_1,
  BS_STATE_2,
  BS_STATE_3,
  BS_PROBLEM_1,
  BS_PROBLEM_2,
  BS_PROBLEM_3,
} bs_iter_state_t;

typedef enum {
  BS_DO_INCREMENT,
  BS_DONT_INCREMENT,
} bs_last_increment_policy_t;

typedef struct bs_splice_t {
  struct bs_splice_t *next;
  struct bs_splice_t *prev;
  int                start;
  int                deleted;
  int                len; // Length of bytes field
  char              *bytes;
} bs_splice_t;

typedef struct {
  bs_splice_t *first;
  bs_splice_t *last;
  char        *bytes;
  int          len;
} buff_string_t;

typedef struct {
  bs_splice_t   *next;
  unsigned int   offset;
  bool           in_splice;
  buff_string_t *str;
} buff_string_iter_t;

void free_buff_string(buff_string_t *str);
int bs_length(buff_string_t *str);
bool bs_takewhile(buff_string_iter_t *iter, char *out, bool (*p)(char));
void bs_index(buff_string_t *str, buff_string_iter_t *iter, int i);
void bs_begin(buff_string_iter_t *iter, buff_string_t *str);
void bs_end(buff_string_iter_t *iter, buff_string_t *str);
bool bs_find(buff_string_iter_t *iter, bool (*p)(char));
bool bs_find_back(buff_string_iter_t *iter, bool (*p)(char));
bool bs_move(buff_string_iter_t *iter, int dx);
int bs_offset(buff_string_iter_t *iter);
void bs_insert(buff_string_iter_t *iter, bs_direction_t dir, char *str, int deleted, ...);
void bs_forward_word(buff_string_iter_t *iter);
void bs_backward_word(buff_string_iter_t *iter);

bool bs_iterate(
  buff_string_iter_t        *iter,
  bs_direction_t             dir,
  bs_last_increment_policy_t last_inc,
  bool                       (*p)(char)
);

// Deallocate memory with `free`
bs_splice_t *new_splice(int len);
bs_splice_t *new_splice_str(char *str);
