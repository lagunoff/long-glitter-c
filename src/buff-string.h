#pragma once
#include <stdbool.h>

typedef enum {
  BS_RIGHT,
  BS_LEFT,
} bs_direction_t;

typedef enum {
  BS_DO_INCREMENT,
  BS_DONT_INCREMENT,
} bs_last_increment_policy_t;

struct buff_string_t;

typedef struct {
  int                   index;
  struct buff_string_t *value;
} bs_index_pair_t;

typedef struct {
  struct buff_string_t *base;
  int   start;
  int   deleted;
  int   len;
  char *bytes;
} bs_splice_t;

typedef struct {
  char *bytes;
  int   len;
} bs_bytes_t;

//! TODO: need to balance this structure somehow, later found
//! https://en.wikipedia.org/wiki/Rope_(data_structure) need more
//! research on how to represent strings in text editors
struct buff_string_t {
  enum {
    BS_SPLICE,
    BS_BYTES,
  } tag;
  union {
    bs_splice_t splice;
    bs_bytes_t  bytes;
  };
};
typedef struct buff_string_t buff_string_t;

typedef struct {
  buff_string_t **str;
  int   local_index;
  int   global_index;
  char *bytes;
  int   begin;
  int   end;
} buff_string_iter_t;

bool bs_takewhile(buff_string_iter_t *iter, char *out, bool (*p)(char));
bool bs_take(buff_string_iter_t *iter, char *out, int n);
int bs_take_2(buff_string_iter_t *iter, char *out, int n);
void bs_index(buff_string_t **str, buff_string_iter_t *iter, int i);
void bs_begin(buff_string_iter_t *iter, buff_string_t **str);
void bs_end(buff_string_iter_t *iter, buff_string_t **str);
bool bs_find(buff_string_iter_t *iter, bool (*p)(char));
bool bs_find_back(buff_string_iter_t *iter, bool (*p)(char));
bool bs_move(buff_string_iter_t *iter, int dx);
int bs_diff(buff_string_iter_t *a, buff_string_iter_t *b);
int bs_offset(buff_string_iter_t *iter);
buff_string_t *bs_insert(buff_string_t *base, int start, char *str, int deleted, bs_direction_t dir, ...);
buff_string_t *bs_insert_undo(buff_string_t *base, ...);
void bs_forward_word(buff_string_iter_t *iter);
void bs_backward_word(buff_string_iter_t *iter);

bool bs_iterate(
  buff_string_iter_t        *iter,
  bs_direction_t             dir,
  bs_last_increment_policy_t last_inc,
  bool                       (*p)(char)
);

void bs_free(buff_string_t *str, void (*free_last) (bs_bytes_t *));
buff_string_t *new_bytes(char *str, int len);
buff_string_t *new_splice(int start, int len, int deleted, buff_string_t *base);
buff_string_t *new_splice_str(char *str, int start, int deleted, buff_string_t *base);
