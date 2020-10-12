#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "buff-string.h"
#include "main.h"

#define DESTRUCT_ITER(iter) \
  char *end = (iter)->str->bytes + (iter)->str->len;\
  struct splice *next = *(iter)->next;\
  struct splice *prev = next ? next->prev : (iter)->str->last;\
  char *base_start = prev ? (iter)->str->bytes + (prev->start + prev->deleted) : (iter)->str->bytes;\
  char *base_end = next ? (iter)->str->bytes + next->start : end\

#define ITER_CASE_1(iter) \
  if ((iter)->current >= base_start && (next ? (iter)->current < base_end : (iter)->current <= base_end))

#define ITER_CASE_2(iter) \
  if (next && (iter)->current >= next->bytes && (iter)->current < (next->bytes + next->len))

#define ITER_ABSURD(iter) {\
  debug0("Iter invariant violation!");      \
  inspect_iter(iter);                       \
  exit(EXIT_FAILURE);                       \
}

#define inspect_splice(splice) {     \
  inspect(%lu, (splice)->bytes);   \
  inspect(%d, (splice)->start);     \
  inspect(%d, (splice)->len);       \
  inspect(%d, (splice)->deleted);   \
}

#define inspect_iter(iter) {                            \
  inspect(%lu, (iter)->current);                      \
  inspect(%lu, *(iter)->next);                          \
  inspect(%lu, (iter)->str->bytes);                     \
  inspect(%d, (iter)->current - (iter)->str->bytes);    \
  if (*(iter)->next) inspect_splice(*(iter)->next);     \
  if ((*(iter)->next)->prev) inspect_splice((*(iter)->next)->prev);     \
}

#define CHECK_ITER(iter) {                                              \
  struct splice *next = *(iter)->next;                                  \
  struct splice *prev = next ? next->prev : (iter)->str->last;          \
  char *base_start = (iter)->str->bytes;                                \
  char *base_end = (iter)->str->bytes + (iter)->str->len;               \
  bool inside_base = (iter)->current >= base_start && (iter)->current < base_end; \
  if (prev && inside_base) {                                            \
    char *prev_end = (iter)->str->bytes + prev->start + prev->deleted;  \
    if (!((iter)->current >= prev_end)) inspect_iter(iter);             \
    assert0((iter)->current >= prev_end);                               \
  }                                                                     \
  if (next) {                                                           \
    char *base_end = (iter)->str->bytes + next->start;                  \
    char *next_start = next->bytes;                                     \
    char *next_end = next->bytes + next->len;                           \
    bool inside_next = (iter)->current >= next_start && (iter)->current < next_end; \
    if (inside_base) {                                                  \
      if (!((iter)->current < base_end)) inspect_iter(iter);            \
      assert0((iter)->current < base_end);                              \
    }                                                                   \
    if (!(inside_base || inside_next)) inspect_iter(iter);              \
    assert0(inside_base || inside_next);                                \
  }                                                                     \
}

void _buff_string_insert_after_1(struct buff_string *str, struct splice *anchor, struct splice *new);
void _buff_string_insert_after_cursor(struct buff_string_iter *iter, struct splice *anchor, struct splice *new);
void _buff_string_insert_after_iter(struct buff_string_iter *iter, struct splice *anchor, struct splice *new);
void _buff_string_iter_right(struct buff_string_iter *iter);
int _buff_string_iter_left(struct buff_string_iter *iter);

void inspect_splices_deep_2 (struct buff_string_iter *it) {
  char temp[16 * 1024];
  int i = 0;
  struct splice *iter = *it->next;
  for (;iter; iter = iter->next) {
    memcpy(temp + i, iter->bytes, iter->len);
    i += iter->len;
    i += sprintf(temp + i, ":%d:%d | ", iter->start, iter->len);
  }
  temp[i] ='\0';
  fprintf(stderr, "%s\n", temp);
}

void inspect_splices_deep (struct buff_string *str) {
  char temp[16 * 1024];
  int i = 0;
  struct splice *iter = str->first;
  for (;iter; iter = iter->next) {
    memcpy(temp + i, iter->bytes, iter->len);
    i += iter->len;
    i += sprintf(temp + i, ":%d:%d | ", iter->start, iter->len);
  }
  temp[i] ='\0';
  fprintf(stderr, "%s\n", temp);
}

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
  int g = 0;
  while(1) {
    assert0(g++ < 100);
    struct splice *next = *iter->next;
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
      iter->next = &next->next;
    }
  }
}

struct splice *new_splice(int len) {
  struct splice *s = malloc(sizeof(struct splice));
  memset(s, 0, sizeof(struct splice));
  s->bytes = malloc(len);
  memset(s->bytes, 0, len);
  s->len = len;
  return s;
}

struct splice *new_splice_str(char *str) {
  int len = strlen(str);
  struct splice *result = new_splice(len);
  memcpy(result->bytes, str, len);
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

bool buff_string_find(struct buff_string_iter *iter, bool (*p)(char)) {
  int i=0;
  while(1) {
    assert0(i++ < 100);
    DESTRUCT_ITER(iter);
    if (buff_string_is_end(iter)) goto eof;

    ITER_CASE_1(iter) {
      //      debug0("ITER_CASE_1");
      for(;iter->current < base_end;iter->current++) {
        if (p(*(iter->current))) goto found;
      }
      debug0("_buff_string_iter_righ");
      _buff_string_iter_right(iter);
      continue;
    }

    ITER_CASE_2(iter) {
      //      debug0("ITER_CASE_2");
      // iter->current points to string inside next
      for(;iter->current < next->bytes + next->len; iter->current++) {
        if (p(*(iter->current))) goto found;
      }
      debug0("_buff_string_iter_righ");
      _buff_string_iter_right(iter);
      continue;
    }

    ITER_ABSURD(iter);
  }

  found:
  //  debug0("FOUND!");
  CHECK_ITER(iter);
  return true;

  eof:
  //  debug0("NOT FOUND!");
  CHECK_ITER(iter);
  return false;
}

bool buff_string_find_back(struct buff_string_iter *iter, bool (*p)(char)) {
  int i = 0;
  while(1) {
    assert0(i++ < 100);
    DESTRUCT_ITER(iter);
    if (buff_string_is_begin(iter)) goto eof;

    ITER_CASE_1(iter) {
      // iter->current points to string between prev and next
      for(;iter->current >= base_start;iter->current--) {
        if (p(*(iter->current))) goto found;
      }
      _buff_string_iter_left(iter);
      continue;
    }

    ITER_CASE_2(iter) {
      // iter->current points to string inside next
      for(;iter->current >= next->bytes;iter->current--) {
        if (p(*(iter->current))) goto found;
      }
      _buff_string_iter_left(iter);
      continue;
    }

    ITER_ABSURD(iter);
  }

  found:
  CHECK_ITER(iter);
  return true;

  eof:
  CHECK_ITER(iter);
  return false;
}

void buff_string_takewhile(struct buff_string_iter *iter, char *dest, bool (*p)(char)) {
  int j=0;
  int i = 0;
  while(1) {
    assert0(i++ < 100);
    DESTRUCT_ITER(iter);
    if (iter->current == end) goto eof;

    ITER_CASE_1(iter) {
      // iter->current points to string between iter->prev and next
      for(;iter->current < base_end;iter->current++) {
        if (!p(*(iter->current))) goto pred_end;
        dest[j++] = *(iter->current);
      }
      debug0("_buff_string_iter_righ");
      _buff_string_iter_right(iter);
      continue;
    }

    ITER_CASE_2(iter) {
      // iter->current points to string inside next
      for(;iter->current < next->bytes + next->len;iter->current++) {
        if (!p(*(iter->current))) goto pred_end;
        dest[j++] = *(iter->current);
      }
      debug0("_buff_string_iter_righ");
      _buff_string_iter_right(iter);
      continue;
    }

    ITER_ABSURD(iter);
  }

  pred_end:
  CHECK_ITER(iter);
  dest[j]='\0';
  return;

  eof:
  CHECK_ITER(iter);
  buff_string_end(iter, iter->str);
  dest[j]='\0';
  return;
}

void buff_string_begin(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->first && str->first->start==0 ? str->first->bytes : str->bytes;
  iter->next = &str->first;
}

bool buff_string_is_begin(struct buff_string_iter *iter) {
  if (iter->str->first && iter->str->first->start==0) {
    return iter->current == iter->str->first->bytes;
  }
  return iter->current == iter->str->bytes;
}

void buff_string_end(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->bytes + str->len;
  iter->next = str->last ? &str->last->next : &str->last;
}

bool buff_string_is_end(struct buff_string_iter *iter) {
  return iter->current == iter->str->bytes + iter->str->len;
}

void _buff_string_iter_right(struct buff_string_iter *iter) {
  debug0("ENTERED");
  int i = 0;
  while(1) {
    assert0(i++ < 100);

    DESTRUCT_ITER(iter);

    if (buff_string_is_end(iter)) goto end;

    if ((iter)->current >= base_start - 1 && (iter)->current <= base_end) {
      debug0("ITER_CASE_1");
      iter->current = next ? next->bytes : base_end;
      if (next && next->len == 0) continue;
      goto end;
    }

    if (next && (iter)->current >= next->bytes - 1 && (iter)->current <= (next->bytes + next->len)) {
      debug0("ITER_CASE_2");
      struct splice *nnext = next->next;
      iter->current = iter->str->bytes + (next->start + next->deleted);
      iter->next = &next->next;
      if (nnext && (iter->current == iter->str->bytes + nnext->start)) {
        debug0("if (nnext && (iter->current == iter->str->bytes + nnext->start)) {");
        continue;
      }
      goto end;
    }

    ITER_ABSURD(iter);
  }

 end:
  debug0("ENDED");
}

int _buff_string_iter_left(struct buff_string_iter *iter) {
  int i = 0;
  while(1) {
    assert0(i++ < 100);

    DESTRUCT_ITER(iter);
    if (iter->current == iter->str->bytes - 1) {
      buff_string_begin(iter, iter->str);
      return 0;
    }

    if ((iter)->current >= base_start - 1 && (iter)->current < base_end) {
      if (prev) {
        iter->next = prev->prev ? &prev->prev->next : &iter->str->first;
        iter->current = prev->bytes + prev->len - 1;
        if (prev->len == 0) continue;
        return 1;
      } else {
        iter->current = iter->str->bytes;
        return 0;
      }
    }

    if (next && (iter)->current >= next->bytes - 1 && (iter)->current < (next->bytes + next->len)) {
      if (base_end == iter->str->bytes) {
        buff_string_begin(iter, iter->str);
        return 0;
      } else {
        iter->current = base_end - 1;
      }
      if (prev && iter->str->bytes + (prev->start + prev->deleted) == base_end) {
        continue;
      }
      return 1;
    }

    ITER_ABSURD(iter);
  }
}

bool buff_string_move(struct buff_string_iter *iter, int dx) {
  if (dx > 0) {
    int i = 0;
    return buff_string_find(iter, lambda(bool _(char c) {
      return !(i++ < dx);
    }));
  }
  if (dx < 0) {
    int i = 0;
    return buff_string_find_back(iter, lambda(bool _(char c) {
      return !(i++ < abs(dx));
    }));
  }
  return true;
}

int buff_string_offset(struct buff_string_iter *iter) {
  struct buff_string_iter iter0 = *iter;
  int offset = 0;
  int i = 0;
  while(1) {
    assert0(i++ < 100);
    DESTRUCT_ITER(&iter0);
    if (buff_string_is_begin(&iter0)) return offset;

    ITER_CASE_1(&iter0) {
      // iter0.current points to string between prev and next
      offset += iter0.current - base_start;
      offset += _buff_string_iter_left(&iter0);
      continue;
    }

    ITER_CASE_2(&iter0) {
      // iter0.current points to string inside next
      offset += iter0.current - next->bytes;
      offset += _buff_string_iter_left(&iter0);
      continue;
    }

    ITER_ABSURD(&iter0);
  }
}

void buff_string_insert(struct buff_string_iter *iter, char *str, int deleted, ...) {
  DESTRUCT_ITER(iter);

  ITER_CASE_1(iter) {
    // iter->current points to string between prev and next
    if (prev && iter->str->bytes + prev->start + prev->deleted == iter->current) {
      int insert_offset = prev->len;
      int insert_len = strlen(str);
      int new_len = prev->len + insert_len - deleted;
      int move_len = prev->len - insert_offset - deleted;
      inspect(%d, insert_offset);
      inspect(%d, prev->len);
      inspect(%d, move_len);
      if (prev->len != new_len) prev->bytes = realloc(prev->bytes, new_len);
      if (insert_len > 0) memcpy(prev->bytes + insert_offset, str, insert_len);
      prev->len = new_len;
      prev->deleted += deleted;
      iter->current = iter->str->bytes + prev->start + prev->deleted;
      goto end;
    }

    struct splice *splice = new_splice_str(str);
    splice->deleted = deleted;
    splice->start = iter->current - iter->str->bytes;
    _buff_string_insert_after_1(iter->str, prev, splice);
    _buff_string_insert_after_cursor(iter, prev, splice);

    va_list iters;
    va_start(iters, deleted);
    while(1) {
      struct buff_string_iter *it = va_arg(iters, struct buff_string_iter *);
      if (it == NULL) break;
      _buff_string_insert_after_iter(it, prev, splice);
    }
    va_end(iters);
    goto end;
  }

  ITER_CASE_2(iter) {
    // iter->current points to string inside next
    int insert_offset = iter->current - next->bytes;
    int insert_len = strlen(str);
    int new_len = next->len + insert_len - deleted;
    int move_len = next->len - insert_offset - deleted;
    inspect(%d, insert_offset);
    inspect(%d, next->len);
    inspect(%d, move_len);
    if (next->len != new_len) next->bytes = realloc(next->bytes, new_len);
    if (move_len > 0) memmove(next->bytes + insert_offset + insert_len + deleted, next->bytes + insert_offset + deleted, move_len);
    if (insert_len > 0) memcpy(next->bytes + insert_offset, str, insert_len);
    iter->current = next->bytes + insert_offset + insert_len;
    next->len = new_len;
    next->deleted += deleted;
    goto end;
  }

  ITER_ABSURD(iter);

  end:
  CHECK_ITER(iter);
}

void _buff_string_insert_after_1(struct buff_string *str, struct splice *anchor, struct splice *new) {
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
  struct splice *iter = str->first;
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

void _buff_string_insert_after_cursor(struct buff_string_iter *iter, struct splice *anchor, struct splice *new) {
  DESTRUCT_ITER(iter);

  if (anchor == prev) {
    printf("if (anchor == prev)\n");
    char *new_start = iter->str->bytes + new->start;
    char *new_end = iter->str->bytes + iter->str->len;
    if (iter->current >= new_start && iter->current < new_end) {
      debug0("if (iter->current >= new_start && iter->current < new_end) {");
      iter->current = new->bytes - 1;
      debug0("_buff_string_iter_righ");
      _buff_string_iter_right(iter);
    }
  }
  CHECK_ITER(iter);
}

void _buff_string_insert_after_iter(struct buff_string_iter *iter, struct splice *anchor, struct splice *new) {
  DESTRUCT_ITER(iter);
  char *new_start = iter->str->bytes + new->start;
  char *new_end = iter->str->bytes + iter->str->len;

  if (anchor == prev) {
    printf("if (anchor == prev)\n");

    if (iter->current >= new_start && iter->current < new_end) {
      debug0("if (iter->current >= new_start && iter->current < new_end) {");
      iter->current = new->bytes - 1;
      iter->next = new->prev ? &new->prev->next : &iter->str->first;
      _buff_string_iter_left(iter);
    }
  }
  CHECK_ITER(iter);
}

void buff_string_unittest() {
  char *s01 = "1234567890abcdefjhijklmnopqrstuvwxyz1234567890";
  char temp[512];
  struct buff_string str01 = {s01, strlen(s01), NULL, NULL};
  struct buff_string_iter iter = BUFF_STRING_BEGIN_INIT(&str01);

  bool found = buff_string_find(&iter, lambda(bool _(char c) {
    return c=='z';
  }));

  assert(found);
  assert(*(iter.current) == 'z');

  buff_string_end(&iter, &str01);
  found = buff_string_find_back(&iter, lambda(bool _(char c) {
     return c=='z';
  }));

  assert(found);
  assert(*(iter.current) == 'z');

  buff_string_begin(&iter, &str01);
  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));
  assert(strcmp(temp, "1234567890")==0);

  struct splice *splice02 = new_splice_str("0000000000");
  splice02->start=0;
  struct buff_string str02 = {s01, strlen(s01), splice02, splice02};

  buff_string_begin(&iter, &str02);

  buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
    return isdigit(c);
  }));

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
