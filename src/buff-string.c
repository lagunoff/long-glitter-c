#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "buff-string.h"
#include "dlist.h"
#include "utils.h"

void _bs_insert_insert_fixup(buff_string_iter_t *iter, buff_string_t *new, bs_direction_t dir);
void _bs_insert_insert_fixup_inv(buff_string_iter_t *iter, buff_string_t *new);

int bs_length(buff_string_t *str) {
  buff_string_iter_t iter;
  bs_end(&iter, &str);
  return iter.global_index;
}

void bs_index(buff_string_t **str, buff_string_iter_t *iter, int i) {
  bs_begin(iter, str);
  bs_move(iter, i);
}

buff_string_t *new_bytes(char *str, int len) {
  buff_string_t *s = malloc(sizeof(buff_string_t));
  s->tag = BuffString_Bytes;
  s->bytes.bytes = str;
  s->bytes.len = len;
  return s;
}

buff_string_t *new_splice(int start, int len, int deleted, buff_string_t *base) {
  buff_string_t *s = malloc(sizeof(buff_string_t));
  s->tag = BuffString_Splice;
  s->splice.bytes = malloc(MAX(len, 1));
  s->splice.len = len;
  s->splice.deleted = deleted;
  s->splice.base = base;
  s->splice.start = start;
  return s;
}

buff_string_t *new_splice_str(char *str, int start, int deleted, buff_string_t *base) {
  int len = strlen(str);
  buff_string_t *result = new_splice(start, len, deleted, base);
  memcpy(result->splice.bytes, str, len);
  return result;
}

void bs_free(buff_string_t *str, void (*free_last) (bs_bytes_t *)) {
  while (1) {
    switch (str->tag) {
    case BuffString_Bytes: {
      free_last(&str->bytes);
      free(str);
      return;
    }
    case BuffString_Splice: {
      buff_string_t *base = str->splice.base;
      free(str->splice.bytes);
      free(str);
      if (base == NULL) return;
      str = base;
      continue;
    }
    }
  }
}

bool bs_iterate(
  buff_string_iter_t        *iter,
  bs_direction_t             dir,
  bs_last_increment_policy_t last_inc,
  bool                       (*p)(char)
) {
  enum {MOVING_INSIDE, LOOKING_NEXT} local_state = LOOKING_NEXT;
  buff_string_t *str;
 to_looking_next:
  str = *iter->str;
  iter->begin = 0;
  iter->end = INT_MAX / 2;
  iter->local_index = iter->global_index;
  local_state = LOOKING_NEXT;
  while (1) {
    switch (local_state) {
    case MOVING_INSIDE: {
      if (dir == BuffString_Right) {
        if (iter->local_index >= iter->end) goto to_looking_next;
        bool is_stop = p(iter->bytes[iter->local_index]);
        if (is_stop && last_inc == BuffString_DontIncrement) goto stopped;
        iter->local_index++;
        iter->global_index++;
        if (is_stop) goto stopped;
      } else {
        if (iter->local_index <= iter->begin) goto to_looking_next;
        bool is_stop = p(iter->bytes[iter->local_index - 1]);
        if (is_stop && last_inc == BuffString_DontIncrement) goto stopped;
        iter->local_index--;
        iter->global_index--;
        if (is_stop) goto stopped;
      }
      continue;
    }
    case LOOKING_NEXT: {
      switch (str->tag) {
      case BuffString_Bytes: {
        if (dir == BuffString_Right && iter->local_index >= MIN(str->bytes.len, iter->end)) goto eof;
        if (dir == BuffString_Left && iter->local_index <= 0) goto eof;
        iter->bytes = str->bytes.bytes;
        iter->begin = iter->begin;
        iter->end = MIN(iter->end, str->bytes.len);
        local_state = MOVING_INSIDE;
        continue;
      }
      case BuffString_Splice: {
        if (dir == BuffString_Right && iter->local_index >= str->splice.start + str->splice.len
            || dir == BuffString_Left && iter->local_index > str->splice.start + str->splice.len) {
          iter->local_index += str->splice.deleted - str->splice.len;
          iter->begin = str->splice.start + str->splice.deleted;
          iter->end += str->splice.deleted - str->splice.len;
          str = str->splice.base;
          continue;
        }
        if (dir == BuffString_Right && iter->local_index < str->splice.start
            || dir == BuffString_Left && iter->local_index <= str->splice.start) {
          iter->end = MIN(iter->end, str->splice.start);
          str = str->splice.base;
          continue;
        }
        iter->bytes = str->splice.bytes;
        iter->begin = MAX(0, iter->begin - str->splice.start);
        iter->end = MIN(iter->end, str->splice.len);
        iter->local_index = iter->local_index - str->splice.start;
        local_state = MOVING_INSIDE;
        continue;
      }
      }
    }
    }
  }
 stopped:
  return false;
 eof:
  return true;
}

bool bs_find(buff_string_iter_t *iter, bool (*p)(char)) {
  return bs_iterate(iter, BuffString_Right, BuffString_DontIncrement, p);
}

bool bs_find_back(buff_string_iter_t *iter, bool (*p)(char)) {
  return bs_iterate(iter, BuffString_Left, BuffString_DoIncrement, p);
}

bool bs_takewhile(buff_string_iter_t *iter, char *dest, bool (*p)(char)) {
  int j = 0;
  bool eof = bs_find(iter, lambda(bool _(char c) {
    bool is_ok = p(c);
    if (is_ok) dest[j++] = c;
    return !is_ok;
  }));
  dest[j] ='\0';
  return eof;
}

bool bs_take(buff_string_iter_t *iter, char *dest, int n) {
  int j = 0;
  bool eof = bs_find(iter, lambda(bool _(char c) {
    bool is_ok = j < n;
    if (is_ok) dest[j++] = c;
    return !is_ok;
  }));
  dest[j] ='\0';
  return eof;
}

int bs_take_2(buff_string_iter_t *iter, char *dest, int n) {
  int j = 0;
  // TODO: too slow, need to copy the string block by block
  bs_find(iter, lambda(bool _(char c) {
    bool is_ok = j < n;
    if (is_ok) dest[j++] = c;
    return !is_ok;
  }));
  return MAX(0, j);
}

int bs_diff(buff_string_iter_t *a, buff_string_iter_t *b) {
  return bs_offset(b) - bs_offset(a);
}

void bs_begin(buff_string_iter_t *iter, buff_string_t **str) {
  iter->str=str;
  iter->global_index = 0;
}

void bs_end(buff_string_iter_t *iter, buff_string_t **str) {
  iter->str=str;
  iter->global_index = 0;
  bs_iterate(iter, BuffString_Right, BuffString_DontIncrement, lambda(bool _(char c) {return false;}));
}

bool bs_move(buff_string_iter_t *iter, int dx) {
  if (dx >= 0) {
    int i = 0;
    return bs_iterate(iter, BuffString_Right, BuffString_DontIncrement, lambda(bool _(char c) {return !(i++ < dx);}));
  }
  if (dx < 0) {
    int i = 0;
    return bs_iterate(iter, BuffString_Left, BuffString_DontIncrement, lambda(bool _(char c) {return !(i++ < abs(dx));}));
  }
  return true;
}

int bs_offset(buff_string_iter_t *iter) {
   return iter->global_index;
}

buff_string_t *bs_insert(buff_string_t *base, int start, char *str, int deleted, bs_direction_t dir, iter_get_fixups_t get_fixups) {
  auto void fixup_cursor(buff_string_iter_t *iter);
  auto void fixup_scroll(buff_string_iter_t *iter);
  int start1 = dir == BuffString_Right ? start : start - deleted;
  // TODO: Check for right bound
  if (start1 < 0) return base;
  buff_string_t *splice = new_splice_str(str, start1, deleted, base);
  if (get_fixups) get_fixups(&fixup_cursor, &fixup_scroll);
  return splice;

  void fixup_cursor(buff_string_iter_t *iter) {
    if (splice->splice.start <= iter->global_index) {
      int dx = splice->splice.len - (dir == BuffString_Left ? splice->splice.deleted : 0);
      bs_move(iter, dx);
    }
  }
  void fixup_scroll(buff_string_iter_t *iter) {
    if (splice->splice.start < iter->global_index) {
      int dx = splice->splice.len - (dir == BuffString_Left ? splice->splice.deleted : 0);
      bs_move(iter, dx);
    }
  }
}

buff_string_t *bs_insert_undo(buff_string_t *base, ...) {
  switch (base->tag) {
  case BuffString_Splice: {
    __auto_type new = &base->splice;
    va_list iters;
    va_start(iters, base);
    while(1) {
      buff_string_iter_t *it = va_arg(iters, buff_string_iter_t *);
      if (it == NULL) break;
      _bs_insert_insert_fixup_inv(it, base);
    }
    va_end(iters);
    buff_string_t *new_base = base->splice.base;
    base->splice.base = NULL;
    bs_free(base, lambda(void _() {}));
    return new_base;
  }
  default: {
    return base;
  }}
}

void _bs_insert_insert_fixup_inv(buff_string_iter_t *iter, buff_string_t *new) {
  if (new->splice.start < iter->global_index) {
    int dx = -MIN(new->splice.len, iter->global_index - new->splice.start) + new->splice.deleted;
    bs_move(iter, dx);
  }
}

void bs_forward_word(buff_string_iter_t *iter) {
  bs_find(iter, lambda(bool _(char c) {return isalnum(c);}));
  bs_find(iter, lambda(bool _(char c) {return !isalnum(c);}));
}

void bs_backward_word(buff_string_iter_t *iter) {
  bs_find_back(iter, lambda(bool _(char c) { return isalnum(c); }));
  bs_find_back(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

char _bs_read_next(buff_string_iter_t *iter, bs_direction_t dir) {
  char ch;
  bs_iterate(iter, dir, BuffString_DontIncrement, lambda(bool _(char c) {ch = c; return true;}));
  return ch;
}

char _bs_current(buff_string_iter_t *iter) {
  _bs_read_next(iter, BuffString_Right);
}

void bs_unittest() {
  char *s01 = "1234567890abcdefjhijklmnopqrstuvwxyz1234567890";
  char temp[512];
  buff_string_t str01 = {.tag = BuffString_Bytes, .bytes = {s01, strlen(s01)}};
  buff_string_t *_str01 = &str01;
  buff_string_iter_t iter;
  bs_begin(&iter, &_str01);

  bool eof = bs_find(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(_bs_current(&iter) == 'z');

  bs_end(&iter, &_str01);
  eof = bs_find_back(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(_bs_current(&iter) == 'z');

  bs_begin(&iter, &_str01);
  bs_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));
  assert(strcmp(temp, "1234567890")==0);

  char *splice02 = "0000000000";
  buff_string_t str02 = {.tag = BuffString_Splice, .splice = {&str01, 0, 0, strlen(splice02), splice02} };
  buff_string_t *_str02 = &str02;

  bs_begin(&iter, &_str02);

  bs_takewhile(&iter, temp, lambda(bool _(char c) {return isdigit(c);}));

  assert(strcmp(temp, "00000000001234567890")==0);
  bs_begin(&iter, &_str02);
  bs_takewhile(&iter, temp, lambda(bool _(char c) {
    return true;
  }));

  assert(strcmp(temp, "00000000001234567890abcdefjhijklmnopqrstuvwxyz1234567890")==0);

  bs_index(&_str02, &iter, 22);

  assert(_bs_current(&iter) == 'c');

  bs_begin(&iter, &_str02);
  bs_find(&iter, lambda(bool _(char c) {
    return c=='k';
  }));

  int offset = bs_offset(&iter);
  assert(offset==30);

  /* free_buff_string(&_str01); */
  /* free_buff_string(&_str02); */
}
