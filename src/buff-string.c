#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>

#include "buff-string.h"
#include "main.h"

bs_iter_state_t _bs_get_iter_state(buff_string_iter_t *iter, bs_direction_t dir);
void _bs_check_iter(buff_string_iter_t *iter);
void _bs_iter_right(buff_string_iter_t *iter);
int _bs_iter_left(buff_string_iter_t *iter);
void bs_insert_inside_splice(char *iter, bs_splice_t *splice, bs_direction_t dir, char *str, int deleted);
void _bs_insert_after_1(buff_string_t *str, bs_splice_t *anchor, bs_splice_t *new);
void _bs_insert_after_cursor(buff_string_iter_t *iter, bs_splice_t *anchor, bs_splice_t *new);
void _bs_insert_after_iter(buff_string_iter_t *iter, bs_splice_t *anchor, bs_splice_t *new);
void bs_splice_overlap(int offset, bs_splice_t *dest, bs_splice_t *splice, bs_direction_t dir);

int bs_length(buff_string_t *str) {
  bs_splice_t *iter = str->first;
  int i=str->len;
  for(;iter;iter=iter->next) {
    i+=iter->len;
    i-=iter->deleted;
  }
  return i;
}

void bs_index(buff_string_t *str, buff_string_iter_t *iter, int i) {
  int j = 0; // Real index
  int k = 0; // Base index
  bs_begin(iter, str);
  while(1) {
    bs_splice_t *next = iter->next;
    if (!next || next->start > (k + (i - j))) {
      // Required index is before iter->next
      iter->offset = (k + (i - j));
      iter->in_splice = false;
      break;
    }

    if ((k + (i - j)) < next->start + next->len) {
      // Required index is inside next
      int offset = (k + (i - j)) - next->start;
      iter->offset = offset;
      iter->in_splice = true;
      break;
    } else {
      j = j + (next->start - k) + next->len;
      k = next->start + next->deleted;
      iter->next = next->next;
    }
  }
}

bs_splice_t *new_splice(int len) {
  bs_splice_t *s = malloc(sizeof(bs_splice_t));
  memset(s, 0, sizeof(bs_splice_t));
  s->bytes = malloc(MAX(len, 1));
  memset(s->bytes, 0, len);
  s->len = len;
  return s;
}

bs_splice_t *new_splice_str(char *str) {
  int len = strlen(str);
  bs_splice_t *result = new_splice(len);
  memcpy(result->bytes, str, MAX(len, 1));
  return result;
}

void free_buff_string(buff_string_t *str) {
  bs_splice_t *iter = str->first;
  for(;iter;) {
    bs_splice_t *temp = iter;
    iter=iter->next;
    free(temp);
  }
}

bool bs_iterate(
  buff_string_iter_t        *iter,
  bs_direction_t             dir,
  bs_last_increment_policy_t last_inc,
  bool                       (*p)(char)
) {
  while(1) {
    bs_iter_state_t state = _bs_get_iter_state(iter, dir);
    bs_splice_t *next = iter->next;
    bs_splice_t *prev = (next == iter->str->first) ? NULL : next ? next->prev : iter->str->last;
    switch(state) {
    case BS_STATE_1: {
      if (dir == BS_RIGHT) {
        if (iter->offset >= iter->str->len) goto eof;
        bool is_stop = p(iter->str->bytes[iter->offset]);
        if (is_stop && last_inc == BS_DONT_INCREMENT) goto stopped;
        iter->offset++;
        if (is_stop) goto stopped;
      } else {
        if (iter->offset <= 0) goto eof;
        bool is_stop = p(iter->str->bytes[iter->offset - 1]);
        if (is_stop && last_inc == BS_DO_INCREMENT) iter->offset--;
        if (is_stop) goto stopped;
        iter->offset--;
      }
      continue;
    }
    case BS_STATE_2: {
      if (dir == BS_RIGHT) {
        bool is_stop = p(next->bytes[iter->offset]);
        if (is_stop && last_inc == BS_DO_INCREMENT) iter->offset++;
        if (is_stop) goto stopped;
        iter->offset++;
      } else if (dir == BS_LEFT) {
        bool is_stop = p(next->bytes[iter->offset - 1]);
        if (is_stop && last_inc == BS_DO_INCREMENT) iter->offset--;
        if (is_stop) goto stopped;
        iter->offset--;
      }
      continue;
    }
    case BS_PROBLEM_1: {
      iter->next = prev;
      continue;
    }
    case BS_PROBLEM_2: {
      iter->next = next->next;
      continue;
    }
    case BS_PROBLEM_3: {
      bs_splice_t *splice = dir == BS_RIGHT ? iter->next : prev;
      int start_offset = iter->offset - iter->next->start;
      iter->offset = MIN(start_offset, iter->next->len);
      iter->next = splice;
      iter->in_splice = true;
      continue;
    }
    case BS_PROBLEM_4: {
      if (dir == BS_RIGHT) {
        iter->offset = next->start + next->deleted + (iter->offset - next->len);
        iter->in_splice = false;
        iter->next = next->next;
      } else {
        iter->offset = next->start;
        iter->in_splice = false;
      }
      continue;
    }
    }
  }
  assert(false);

 stopped:
  return false;
 eof:
  return true;
}

bool bs_find(buff_string_iter_t *iter, bool (*p)(char)) {
  return bs_iterate(iter, BS_RIGHT, BS_DONT_INCREMENT, p);
}

bool bs_find_back(buff_string_iter_t *iter, bool (*p)(char)) {
  return bs_iterate(iter, BS_LEFT, BS_DO_INCREMENT, p);
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

void bs_begin(buff_string_iter_t *iter, buff_string_t *str) {
  iter->str=str;
  iter->in_splice = str->first && str->first->start == 0 ? true : false;
  iter->offset = 0;
  iter->next = str->first;
}

void bs_end(buff_string_iter_t *iter, buff_string_t *str) {
  iter->str=str;
  iter->in_splice=false;
  iter->offset = str->len;
  iter->next = str->last;
}

bool bs_move(buff_string_iter_t *iter, int dx) {
  if (dx > 0) {
    int i = 0;
    return bs_iterate(iter, BS_RIGHT, BS_DONT_INCREMENT, lambda(bool _(char c) {return !(i++ < dx);}));
  }
  if (dx < 0) {
    int i = 0;
    return bs_iterate(iter, BS_LEFT, BS_DONT_INCREMENT, lambda(bool _(char c) {return !(i++ < abs(dx));}));
  }
  return true;
}

int bs_offset(buff_string_iter_t *iter1) {
  buff_string_iter_t iter0 = *iter1;
  buff_string_iter_t *iter = &iter0;

  int offset = 0;
  while(1) {
    bs_iter_state_t state = _bs_get_iter_state(iter, BS_LEFT);
    bs_splice_t *next = iter->next;
    bs_splice_t *prev = (next == iter->str->first) ? NULL : (next ? next->prev : iter->str->last);
    switch(state) {
    case BS_STATE_1: {
      if (prev) {
        offset += iter->offset - (prev->start + prev->deleted);
        iter->next = prev;
        iter->in_splice = true;
        iter->offset = prev->len;
        continue;
      } else {
        offset += iter->offset;
        return offset;
      }
    }
    case BS_STATE_2: {
      offset += iter->offset;
      iter->offset = next->start;
      iter->in_splice = false;
      if (prev) iter->next = prev;
      continue;
    }
    case BS_PROBLEM_1: {
      iter->next = prev;
      continue;
    }
    case BS_PROBLEM_2: {
      iter->next = next->next;
      continue;
    }
    case BS_PROBLEM_3: {
      bs_splice_t *splice = prev;
      int start_offset = iter->offset - iter->next->start;
      iter->offset = MIN(start_offset, iter->next->len);
      iter->next = splice;
      iter->in_splice = true;
      continue;
    }
    case BS_PROBLEM_4: {
      iter->offset = next->start;
      iter->in_splice = false;
      continue;
    }
    }
  }
 end:
  return offset;
}

void bs_insert(buff_string_iter_t *iter, bs_direction_t dir, char *str, int deleted, ...) {
  bs_splice_t new_splice = {NULL, NULL, 0, deleted, strlen(str), str};
  while(1) {
    bs_iter_state_t state = _bs_get_iter_state(iter, dir);
    bs_splice_t *next = iter->next;
    bs_splice_t *prev = (next == iter->str->first) ? NULL : (next ? next->prev : iter->str->last);
    switch(state) {
    case BS_STATE_1: {
      // Finding splice next to 'current'
      if (prev && iter->offset < prev->start + prev->deleted) {
        iter->next = prev;
        continue;
      }
      if (dir == BS_LEFT && prev && prev->start + prev->deleted == iter->offset) {
        bs_splice_overlap(prev->len, prev, &new_splice, dir);
        goto end;
      }
      if (dir == BS_RIGHT && next && iter->offset == next->start) {
        bs_splice_overlap(0, next, &new_splice, dir);
        goto end;
      }

      bs_splice_t *splice = new_splice_str(str);
      splice->deleted = deleted;
      splice->start = dir == BS_RIGHT ? iter->offset : iter->offset - splice->deleted;

      _bs_insert_after_1(iter->str, prev, splice);
      //    _bs_insert_after_cursor(iter, prev, splice);

      va_list iters;
      va_start(iters, deleted);
      while(1) {
        buff_string_iter_t *it = va_arg(iters, buff_string_iter_t *);
        if (it == NULL) break;
      }
      va_end(iters);
      goto end;
    }
    case BS_STATE_2: {
      // iter->current points to string inside next
      bs_splice_overlap(iter->offset, next, &new_splice, dir);
      goto end;
    }
    case BS_PROBLEM_1: {
      iter->next = prev;
      continue;
    }
    case BS_PROBLEM_2: {
      iter->next = next->next;
      continue;
    }
    case BS_PROBLEM_3: {
      bs_splice_t *splice = dir == BS_RIGHT ? iter->next : prev;
      int start_offset = iter->offset - iter->next->start;
      iter->offset = MIN(start_offset, iter->next->len);
      iter->next = splice;
      iter->in_splice = true;
      continue;
    }
    case BS_PROBLEM_4: {
      if (dir == BS_RIGHT) {
        iter->offset = next->start + next->deleted + (iter->offset - next->len);
        iter->in_splice = false;
        iter->next = next->next;
      } else {
        iter->offset = next->start;
        iter->in_splice = false;
      }
      continue;
    }
    }
  }

  end:
  _bs_check_iter(iter);
}

void _bs_insert_after_1(buff_string_t *str, bs_splice_t *anchor, bs_splice_t *new) {
  if (anchor == NULL) {
    new->next = str->first;
    if (str->first) {
      str->first->prev = new;
    } else {
      str->last = new;
    }
    str->first = new;
    return;
  }
  bs_splice_t *iter = str->first;
  for (;iter;iter=iter->next) {
    if (iter == anchor) {
      new->next = iter->next;
      new->prev = iter;
      if (iter->next) {
        iter->next->prev = new;
      } else {
        str->last = new;
      }
      iter->next = new;
    }
  }
}

void bs_splice_overlap(int offset, bs_splice_t *dest, bs_splice_t *splice, bs_direction_t dir) {
  if (dir == BS_LEFT) {
    int delta_start = offset - splice->deleted;
    if (delta_start < 0) {
      int new_len = dest->len + splice->len - offset;
      char *new_bytes = new_len != dest->len ? realloc(dest->bytes, MAX(new_len, 1)) : dest->bytes;
      char *move_dest = new_bytes + splice->len;
      char *move_src = new_bytes + offset;
      int move_len = dest->len - offset;
      if (move_src != move_dest && move_len > 0) memmove(move_dest, move_src, move_len);
      memcpy(new_bytes, splice->bytes, splice->len);
      dest->len = new_len;
      dest->deleted = dest->deleted - delta_start;
      dest->bytes = new_bytes;
      dest->start += delta_start;
    } else {
      int new_len = dest->len + splice->len - splice->deleted;
      char *new_bytes = new_len != dest->len ? realloc(dest->bytes, MAX(new_len, 1)) : dest->bytes;
      char *move_dest = new_bytes + offset - splice->deleted + splice->len;
      char *move_src = new_bytes + offset;
      char *cpy_dest = new_bytes + (offset - splice->deleted);
      int move_len = new_len - (offset - splice->deleted + splice->len);
      if (move_src != move_dest && move_len > 0) memmove(move_dest, move_src, move_len);
      memcpy(cpy_dest, splice->bytes, splice->len);
      dest->len = new_len;
      dest->bytes = new_bytes;
    }
  } else { // (dir == BS_RIGHT) {
    int delta_start = splice->deleted - (dest->len - offset);
    if (delta_start >= 0) {
      int new_len = splice->len + offset;
      char *new_bytes = new_len != dest->len ? realloc(dest->bytes, MAX(new_len, 1)) : dest->bytes;
      char *cpy_dest = new_bytes + offset;
      memcpy(cpy_dest, splice->bytes, splice->len);
      dest->len = new_len;
      dest->deleted = dest->deleted + delta_start;
      dest->bytes = new_bytes;
    } else {
      int new_len = dest->len + splice->len - splice->deleted;
      char *new_bytes = new_len != dest->len ? realloc(dest->bytes, MAX(new_len, 1)) : dest->bytes;
      char *move_dest = new_bytes + offset + splice->len;
      char *move_src = new_bytes + offset + splice->deleted;
      int move_len = new_len - (offset + splice->len);
      char *cpy_dest = new_bytes + offset;
      if (move_src != move_dest && move_len > 0) memmove(move_dest, move_src, move_len);
      memcpy(cpy_dest, splice->bytes, splice->len);
      dest->len = new_len;
      dest->bytes = new_bytes;
    }
  }
}

void bs_forward_word(buff_string_iter_t *iter) {
  bs_find(iter, lambda(bool _(char c) { return isalnum(c); }));
  bs_find(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

void bs_backward_word(buff_string_iter_t *iter) {
  bs_find_back(iter, lambda(bool _(char c) { return isalnum(c); }));
  bs_find_back(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

void _bs_check_iter(buff_string_iter_t *iter) {

}

char _bs_read_next(buff_string_iter_t *iter, bs_direction_t dir) {
  char ch;
  bs_iterate(iter, dir, BS_DONT_INCREMENT, lambda(bool _(char c) {ch = c; return true;}));
  return ch;
}

char _bs_current(buff_string_iter_t *iter) {
  _bs_read_next(iter, BS_RIGHT);
}

bs_iter_state_t _bs_get_iter_state(buff_string_iter_t *iter, bs_direction_t dir) {
  bs_splice_t *next = iter->next;
  bs_splice_t *prev = (next == iter->str->first) ? NULL : (next ? next->prev : iter->str->last);

  if (iter->in_splice) {
    return BS_STATE_2;
  }
  if (iter->in_splice && dir == BS_RIGHT && iter->offset >= next->len) {
    return BS_PROBLEM_4;
  }
  if (iter->in_splice && dir == BS_LEFT && iter->offset <= 0) {
    return BS_PROBLEM_4;
  }
  if (!iter->in_splice && next && dir == BS_RIGHT && iter->offset >= next->start) {
    return BS_PROBLEM_3;
  }
  if (!iter->in_splice && next && dir == BS_LEFT && iter->offset <= next->start + next->deleted) {
    return BS_PROBLEM_3;
  }
  if (!iter->in_splice && prev && dir == BS_LEFT && iter->offset <= prev->start + prev->deleted) {
    return BS_PROBLEM_1;
  }
  if (!iter->in_splice && prev && dir == BS_RIGHT && iter->offset < prev->start + prev->deleted) {
    return BS_PROBLEM_1;
  }
  if (!iter->in_splice && !next) {
    return BS_STATE_1;
  }
  if (!iter->in_splice && next && dir == BS_RIGHT && iter->offset < next->start) {
    return BS_STATE_1;
  }
  if (!iter->in_splice && next && dir == BS_LEFT && iter->offset <= next->start) {
    return BS_STATE_1;
  }
  if (!iter->in_splice && next && dir == BS_RIGHT && iter->offset >= (next->start + next->deleted)) {
    return BS_PROBLEM_2;
  }
  if (!iter->in_splice && next && dir == BS_LEFT && iter->offset > (next->start + next->deleted)) {
    return BS_PROBLEM_2;
  }
  assert(false);
}

void bs_unittest() {
  char *s01 = "1234567890abcdefjhijklmnopqrstuvwxyz1234567890";
  char temp[512];
  buff_string_t str01 = {NULL, NULL, s01, strlen(s01)};
  buff_string_iter_t iter;
  bs_begin(&iter, &str01);

  bool eof = bs_find(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(_bs_current(&iter) == 'z');

  bs_end(&iter, &str01);
  eof = bs_find_back(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(_bs_current(&iter) == 'z');

  bs_begin(&iter, &str01);
  bs_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));
  assert(strcmp(temp, "1234567890")==0);

  bs_splice_t *splice02 = new_splice_str("0000000000");
  splice02->start=0;
  buff_string_t str02 = {splice02, splice02, s01, strlen(s01)};

  bs_begin(&iter, &str02);

  bs_takewhile(&iter, temp, lambda(bool _(char c) {return isdigit(c);}));

  printf("temp=%s\n", temp);
  assert(strcmp(temp, "00000000001234567890")==0);
  bs_begin(&iter, &str02);
  bs_takewhile(&iter, temp, lambda(bool _(char c) {
    return true;
  }));
  printf("temp=%s\n", temp);
  assert(strcmp(temp, "00000000001234567890abcdefjhijklmnopqrstuvwxyz1234567890")==0);

  bs_index(&str02, &iter, 22);
  printf("iter->next=%p\n", iter.next);
  printf("iter->offset=%c\n", _bs_current(&iter));

  assert(_bs_current(&iter) == 'c');

  bs_begin(&iter, &str02);
  bs_find(&iter, lambda(bool _(char c) {
    return c=='k';
  }));

  inspect("%p", str02.first);
  inspect("%p", str02.last);
  int offset = bs_offset(&iter);
  printf("offset=%d\n", offset);
  assert(offset==30);

  free_buff_string(&str01);
  free_buff_string(&str02);
}
