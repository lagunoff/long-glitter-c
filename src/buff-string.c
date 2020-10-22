#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>

#include "buff-string.h"
#include "main.h"

buff_string_iter_state _buff_string_get_iter_state(struct buff_string_iter *iter);
void _buff_string_check_iter(struct buff_string_iter *iter);
void _buff_string_iter_right(struct buff_string_iter *iter);
int _buff_string_iter_left(struct buff_string_iter *iter);
void buff_string_insert_inside_splice(char *iter, struct splice *splice, buff_string_dir dir, char *str, int deleted);

int buff_string_length(struct buff_string *str) {
  struct splice *iter = str->first;
  int i=str->len;
  for(;iter;iter=iter->next) {
    i+=iter->len;
    i-=iter->deleted;
  }
  return i;
}

void buff_string_index(struct buff_string *str, struct buff_string_iter *iter, int i) {
  int j = 0; // Real index
  int k = 0; // Base index
  buff_string_begin(iter, str);
  while(1) {
    struct splice *next = iter->next;
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

struct splice *new_splice(int len) {
  struct splice *s = malloc(sizeof(struct splice));
  memset(s, 0, sizeof(struct splice));
  s->bytes = malloc(MAX(len, 1));
  memset(s->bytes, 0, len);
  s->len = len;
  return s;
}

struct splice *new_splice_str(char *str) {
  int len = strlen(str);
  struct splice *result = new_splice(len);
  memcpy(result->bytes, str, MAX(len, 1));
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

bool buff_string_iterate(
  struct buff_string_iter *iter,
  buff_string_dir dir,
  buff_string_last_increment_policy last_inc,
  bool (*p)(char)
) {
  while(1) {
    buff_string_iter_state state = _buff_string_get_iter_state(iter);
    struct splice *next = iter->next;
    struct splice *prev = (next == iter->str->first) ? NULL : next ? next->prev : iter->str->last;
    switch(state) {
    case BSI_STATE_ONE: {
      // Finding splice next to 'current'
      if (prev && iter->offset < prev->start + prev->deleted) {
        iter->next = prev;
        continue;
      } else {
         if (dir == BSD_RIGHT) {
           if (next && iter->offset == next->start) {
             iter->offset = 0;
             iter->in_splice = true;
             continue;
           }
           if (iter->offset >= iter->str->len) goto eof;
           bool is_stop = p(iter->str->bytes[iter->offset]);
           if (is_stop && last_inc == BSLIP_DONT) goto stopped;
           iter->offset++;
           if (is_stop) goto stopped;
         } else if (dir == BSD_LEFT) {
           if (prev && iter->offset == prev->start + next->deleted) {
             iter->offset = prev->len;
             iter->in_splice = true;
             iter->next = prev;
             continue;
           }
           if (iter->offset <= 0) goto eof;
           bool is_stop = p(iter->str->bytes[iter->offset - 1]);
           if (is_stop && last_inc == BSLIP_DO) iter->offset--;
           if (is_stop) goto stopped;
           iter->offset--;
         }
         continue;
      }
      break;
    }
    case BSI_STATE_TWO: {
      // Finding splice next to 'current'
      iter->next = next->next;
      continue;
    }
    case BSI_STATE_THREE: {
      int start_offset = iter->offset - iter->next->start;
      iter->in_splice = true;
      iter->offset = MIN(start_offset, iter->next->len);
      continue;
    }
    case BSI_STATE_FOUR: {
      if (dir == BSD_RIGHT) {
        if (iter->offset >= next->len) {
          iter->in_splice = false;
          iter->offset = next->start + next->deleted;
          iter->next = next->next;
          continue;
        }
        bool is_stop = p(next->bytes[iter->offset]);
        if (is_stop && last_inc == BSLIP_DO) iter->offset++;
        if (is_stop) goto stopped;
        iter->offset++;
      } else if (dir == BSD_LEFT) {
        if (iter->offset <= 0) {
          iter->in_splice = false;
          iter->offset = next->start - 1;
          iter->next = next->prev;
          continue;
        }
        bool is_stop = p(next->bytes[iter->offset - 1]);
        if (is_stop && last_inc == BSLIP_DO) iter->offset--;
        if (is_stop) goto stopped;
        iter->offset--;
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

bool buff_string_find(struct buff_string_iter *iter, bool (*p)(char)) {
  return buff_string_iterate(iter, BSD_RIGHT, BSLIP_DONT, p);
}

bool buff_string_find_back(struct buff_string_iter *iter, bool (*p)(char)) {
  return buff_string_iterate(iter, BSD_LEFT, BSLIP_DO, p);
}

bool buff_string_takewhile(struct buff_string_iter *iter, char *dest, bool (*p)(char)) {
  int j = 0;
  bool eof = buff_string_find(iter, lambda(bool _(char c) {
    bool is_ok = p(c);
    if (is_ok) dest[j++] = c;
    return !is_ok;
  }));
  dest[j] ='\0';
  return eof;
}

void buff_string_begin(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->in_splice = str->first && str->first->start == 0 ? true : false;
  iter->offset = 0;
  iter->next = str->first;
}

void buff_string_end(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->in_splice=false;
  iter->offset = str->len;
  iter->next = str->last;
}

bool buff_string_move(struct buff_string_iter *iter, int dx) {
  if (dx > 0) {
    int i = 0;
    return buff_string_iterate(iter, BSD_RIGHT, BSLIP_DONT, lambda(bool _(char c) {return !(i++ < dx);}));
  }
  if (dx < 0) {
    int i = 0;
    return buff_string_iterate(iter, BSD_LEFT, BSLIP_DONT, lambda(bool _(char c) {return !(i++ < abs(dx));}));
  }
  return true;
}

int buff_string_offset(struct buff_string_iter *iter1) {
  struct buff_string_iter iter0 = *iter1;
  struct buff_string_iter *iter = &iter0;

  int offset = 0;
  while(1) {
    buff_string_iter_state state = _buff_string_get_iter_state(iter);
    struct splice *next = iter->next;
    struct splice *prev = (next == iter->str->first) ? NULL : (next ? next->prev : iter->str->last);
    switch(state) {
    case BSI_STATE_ONE: {
      // Finding splice next to 'current'
      if (prev && iter->offset < prev->start + prev->deleted) {
        iter->next = prev;
        continue;
      }
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
    case BSI_STATE_TWO: {
      // Finding splice next to 'current'
      iter->next = next->next;
      continue;
    }
    case BSI_STATE_THREE: {
      int start_offset = iter->offset - iter->next->start;
      iter->offset = MIN(start_offset, iter->next->len);
      iter->in_splice = true;
      continue;
    }
    case BSI_STATE_FOUR: {
      offset += iter->offset;
      iter->offset = iter->next->start;
      iter->in_splice = false;
      if (prev) iter->next = prev;
      continue;
    }
    }
  }
 end:
  return offset;
}

void buff_string_insert(struct buff_string_iter *iter, buff_string_dir dir, char *str, int deleted, ...) {
}


/** Assuming that iter points inside splice->bytes */
void buff_string_insert_inside_splice(char *iter, struct splice *splice, buff_string_dir dir, char *str, int deleted) {
  int insert_offset = iter - splice->bytes + (dir == BSD_LEFT ? -deleted : 0);
  int insert_len = strlen(str);
  int new_len = splice->len + insert_len - deleted;
  int move_len = splice->len - insert_offset - deleted;
  char *new_bytes = splice->bytes;

  if (dir == BSD_LEFT && insert_offset < 0) return;

  if (splice->len != new_len) {
    new_bytes = realloc(splice->bytes, MAX(new_len, 1));
  }

  char *move_dest = new_bytes + insert_offset + insert_len ;
  char *move_src = new_bytes + insert_offset + deleted;

  if (move_src != move_dest && move_len > 0) memmove(move_dest, move_src, move_len);
  if (insert_len > 0) memcpy(move_dest - insert_len, str, insert_len);
  splice->len = MAX(new_len, 0);
  splice->deleted += MAX(-new_len, 0);
  splice->bytes = new_bytes;
}

void buff_string_forward_word(struct buff_string_iter *iter) {
  buff_string_find(iter, lambda(bool _(char c) { return isalnum(c); }));
  buff_string_find(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

void buff_string_backward_word(struct buff_string_iter *iter) {
  buff_string_find_back(iter, lambda(bool _(char c) { return isalnum(c); }));
  buff_string_find_back(iter, lambda(bool _(char c) { return !isalnum(c); }));
}

void _buff_string_check_iter(struct buff_string_iter *iter) {

}

char _buff_string_read_next(struct buff_string_iter *iter, buff_string_dir dir) {
  char ch;
  buff_string_iterate(iter, dir, BSLIP_DONT, lambda(bool _(char c) {ch = c; return true;}));
  return ch;
}

char _buff_string_current(struct buff_string_iter *iter) {
  _buff_string_read_next(iter, BSD_RIGHT);
}

buff_string_iter_state _buff_string_get_iter_state(struct buff_string_iter *iter) {
  struct splice *next = iter->next;

  if (iter->in_splice) {
    return BSI_STATE_FOUR;
  }
  if (!iter->in_splice && next && iter->offset >= next->start && iter->offset < next->start + next->deleted) {
    return BSI_STATE_THREE;
  }
  if (!iter->in_splice && iter->offset <= (next ? next->start : iter->str->len)) {
    return BSI_STATE_ONE;
  }
  if (!iter->in_splice && iter->offset >= (next->start + next->deleted)) {
    return BSI_STATE_TWO;
  }
  assert(false);
}

void buff_string_unittest() {
  char *s01 = "1234567890abcdefjhijklmnopqrstuvwxyz1234567890";
  char temp[512];
  struct buff_string str01 = {NULL, NULL, s01, strlen(s01)};
  struct buff_string_iter iter;
  buff_string_begin(&iter, &str01);

  bool eof = buff_string_find(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(_buff_string_current(&iter) == 'z');

  buff_string_end(&iter, &str01);
  eof = buff_string_find_back(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(_buff_string_current(&iter) == 'z');

  buff_string_begin(&iter, &str01);
  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));
  assert(strcmp(temp, "1234567890")==0);

  struct splice *splice02 = new_splice_str("0000000000");
  splice02->start=0;
  struct buff_string str02 = {splice02, splice02, s01, strlen(s01)};

  buff_string_begin(&iter, &str02);

  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {return isdigit(c);}));

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
  printf("iter->offset=%c\n", _buff_string_current(&iter));

  assert(_buff_string_current(&iter) == 'c');

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
