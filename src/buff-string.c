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
    struct splice *next = iter->splice;
    if (!next || next->start > (k + (i - j))) {
      // Required index is before iter->next
      iter->current = iter->str->bytes + (k + (i - j));
      break;
    }

    if ((k + (i - j)) < next->start + next->len) {
      // Required index is inside next
      int offset = (k + (i - j)) - next->start;
      iter->current = next->bytes + offset;
      break;
    } else {
      j = j + (next->start - k) + next->len;
      k = next->start + next->deleted;
      iter->splice = next->next;
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
    switch(state) {
    case BSI_STATE_ONE: {
      // Finding splice next to 'current'
      struct splice *it = iter->splice;
      if (it && it->prev && iter->current < iter->str->bytes + it->prev->start + it->prev->deleted) {
        iter->splice = it->prev;
        continue;
      } else {
         if (dir == BSD_RIGHT) {
          if (iter->current >= iter->str->bytes + iter->str->len) goto eof;
          bool is_stop = p(*iter->current);
          if (is_stop && last_inc == BSLIP_DO) iter->current++;
          if (is_stop) goto stopped;
          iter->current++;
        } else if (dir == BSD_LEFT) {
          if (iter->current <= iter->str->bytes) goto eof;
          bool is_stop = p(*(iter->current - 1));
          if (is_stop && last_inc == BSLIP_DO) iter->current--;
          if (is_stop) goto stopped;
          iter->current--;
        }
        continue;
      }
      break;
    }
    case BSI_STATE_TWO: {
      // Finding splice next to 'current'
      struct splice *it = iter->splice;
      if (it && it->next && iter->current >= iter->str->bytes + it->next->start) {
        iter->splice = it->next;
        continue;
      } else {
        if (dir == BSD_RIGHT) {
          if (iter->current >= iter->str->bytes + iter->str->len) goto eof;
          bool is_stop = p(*iter->current);
          if (is_stop && last_inc == BSLIP_DO) iter->current++;
          if (is_stop) goto stopped;
          iter->current++;
        } else if (dir == BSD_LEFT) {
          bool is_stop = p(*(iter->current - 1));
          if (is_stop && last_inc == BSLIP_DO) iter->current--;
          if (is_stop) goto stopped;
          iter->current--;
        }
        continue;
      }
      break;
    }
    case BSI_STATE_THREE: {
      int start_offset = (iter->current - iter->str->bytes) - iter->splice->start;
      iter->current = iter->splice->bytes + MIN(start_offset, iter->splice->len);
      continue;
    }
    case BSI_STATE_FOUR:
      if (dir == BSD_RIGHT) {
        if (iter->current >= iter->splice->bytes + iter->splice->len) {
          iter->current = iter->str->bytes + iter->splice->start + iter->splice->deleted;
          continue;
        }
        bool is_stop = p(*iter->current);
        if (is_stop && last_inc == BSLIP_DO) iter->current++;
        if (is_stop) goto stopped;
        iter->current++;
      } else if (dir == BSD_LEFT) {
        if (iter->current <= iter->splice->bytes) {
          iter->current = iter->str->bytes + iter->splice->start;
          continue;
        }
        bool is_stop = p(*iter->current);
        if (is_stop && last_inc == BSLIP_DO) iter->current--;
        if (is_stop) goto stopped;
        iter->current--;
      }
      continue;
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
  iter->current = str->first && str->first->start==0 ? str->first->bytes : str->bytes;
  iter->splice = str->first;
}

void buff_string_end(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->bytes + str->len;
  iter->splice = str->last;
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
    switch(state) {
    case BSI_STATE_ONE: {
      // Finding splice next to 'current'
      struct splice *it = iter->splice;
      if (it && it->prev && iter->current < iter->str->bytes + it->prev->start + it->prev->deleted) {
        iter->splice = it->prev;
        continue;
      } else {
        offset += iter->current - iter->str->bytes;
        return offset;
      }
    }
    case BSI_STATE_TWO: {
      // Finding splice next to 'current'
      struct splice *it = iter->splice;
      if (it && it->next && iter->current >= iter->str->bytes + it->next->start) {
        iter->splice = it->next;
        continue;
      } else {
        offset += (iter->current - iter->str->bytes) - iter->splice->start + iter->splice->deleted;
        iter->current = iter->splice->bytes + iter->splice->len;
        continue;
      }
    }
    case BSI_STATE_THREE: {
      int start_offset = (iter->current - iter->str->bytes) - iter->splice->start;
      iter->current = iter->splice->bytes + MIN(start_offset, iter->splice->len);
      continue;
    }
    case BSI_STATE_FOUR: {
      offset += iter->current - iter->splice->bytes;
      iter->current = iter->str->bytes + iter->splice->start;
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

buff_string_iter_state _buff_string_get_iter_state(struct buff_string_iter *iter) {
  struct splice *s = iter->splice;
  if (s && iter->current >= s->bytes && iter->current <= s->bytes + s->len) {
    return BSI_STATE_FOUR;
  }
  if (iter->current >= iter->str->bytes && iter->current <= iter->str->bytes + (s ? s->start : iter->str->len)) {
    return BSI_STATE_ONE;
  }
  if (s && iter->current >= iter->str->bytes + (s->start + s-> deleted) && iter->current <= iter->str->bytes + iter->str->len) {
    return BSI_STATE_TWO;
  }
  if (s && iter->current >= iter->str->bytes + s->start && iter->current < iter->str->bytes + s->start + s->deleted) {
    return BSI_STATE_THREE;
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
  assert(*(iter.current) == 'z');

  buff_string_end(&iter, &str01);
  eof = buff_string_find_back(&iter, lambda(bool _(char c) {return c=='z';}));

  assert(!eof);
  assert(*(iter.current) == 'z');

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
  printf("iter->splice=%p\n", iter.splice);
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
