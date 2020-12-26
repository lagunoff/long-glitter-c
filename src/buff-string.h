#pragma once
#include <stdbool.h>

typedef enum {
  BuffString_Right,
  BuffString_Left,
} bs_direction_t;

typedef enum {
  BuffString_DoIncrement,
  BuffString_DontIncrement,
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
    BuffString_Splice,
    BuffString_Bytes,
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

typedef void (*iter_fixup_t)(buff_string_iter_t *iter);
typedef void (*iter_apply_fixups_t)(iter_fixup_t fix_cursor, iter_fixup_t fix_scroll);

bool bs_takewhile(buff_string_iter_t *iter, char *out, bool (*p)(char));
bool bs_take(buff_string_iter_t *iter, char *out, int n);
int bs_take_2(buff_string_iter_t *iter, char *out, int n);
void bs_index(buff_string_t **str, buff_string_iter_t *iter, int i);
void bs_begin(buff_string_iter_t *iter, buff_string_t **str);
void bs_end(buff_string_iter_t *iter, buff_string_t **str);
bool bs_find(buff_string_iter_t *iter, bool (*p)(char));
bool bs_find_back(buff_string_iter_t *iter, bool (*p)(char));
bool bs_move(buff_string_iter_t *iter, int dx);
int bs_length(buff_string_t *str);
char *bs_copy_stringz(buff_string_t *str);
int bs_diff(buff_string_iter_t *a, buff_string_iter_t *b);
int bs_offset(buff_string_iter_t *iter);
void bs_insert(buff_string_t **bs, int start, char *str, int deleted, bs_direction_t dir, iter_apply_fixups_t fixups);
void bs_insert_undo(buff_string_t **bs, iter_apply_fixups_t fixup);
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
