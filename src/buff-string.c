#include <malloc.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "buff-string.h"
#include "main.h"

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
  int j = 0; // Spliced index
  int k = 0; // Main index
  buff_string_begin(iter, str);
  while(1) {
    if (!iter->next || iter->next->start > (k + (i - j))) {
      // Required index is before iter->next
      iter->current = iter->str->bytes + (k + (i - j));
      break;
    }

    if ((k + (i - j)) < iter->next->start + iter->next->len) {
      // Required index is inside iter->next
      int offset = (k + (i - j)) - iter->next->start;
      iter->current = iter->next->bytes + offset;
      break;
    } else {
      j = j + (iter->next->start - k) + iter->next->len;
      k = iter->next->start + iter->next->deleted;
      iter->prev = iter->next;
      iter->next = iter->next->next;
    }
  }
}

struct splice *new_splice(int len) {
  int alloc_len = pow(2, ceil(log2(len)));
  struct splice *s = malloc(sizeof(struct splice) + alloc_len);
  memset(s, 0, sizeof(struct splice) + alloc_len);
  s->len = len;
  s->alloc_len = alloc_len;
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
  char *end = iter->str->bytes + iter->str->len;
  while(1) {
    if (iter->current == end) goto eof;

    if (iter->prev && iter->current >= iter->prev->bytes && iter->current < iter->prev->bytes + iter->prev->len) {
      // iter->current points to string inside iter->prev
      for(;iter->current < iter->prev->bytes + iter->prev->len;iter->current++) {
        if (p(*(iter->current))) goto pred_end;
      }
      iter->current = iter->str->bytes + (iter->prev->start + iter->prev->deleted);
      continue;
    }

    char *middle_start = iter->prev ? iter->str->bytes + (iter->prev->start + iter->prev->deleted) : iter->str->bytes;
    char *middle_end = iter->next ? iter->str->bytes + iter->next->start : iter->str->bytes + iter->str->len;

    if ((uintptr_t)iter->current >= (uintptr_t)middle_start && (uintptr_t)iter->current < (uintptr_t)middle_end) {
      // iter->current points to string between iter->prev and iter->next
      for(;iter->current < middle_end;iter->current++) {
        if (p(*(iter->current))) goto pred_end;
      }
      iter->current = iter->next ? iter->next->bytes : middle_end;
      continue;
    }

    if (iter->next && iter->current >= iter->next->bytes && iter->current < iter->next->bytes + iter->next->len) {
      // iter->current points to string inside iter->next
      for(;iter->current < iter->next->bytes + iter->next->len;iter->current++) {
        if (p(*(iter->current))) goto pred_end;
      }
      iter->current = iter->str->bytes + (iter->next->start + iter->next->deleted);
      iter->prev = iter->next;
      iter->next = iter->next->next;
      continue;
    }
  }

  pred_end:
  return true;

  eof:
  buff_string_end(iter, iter->str);
  return false;
}

bool buff_string_find_back(struct buff_string_iter *iter, bool (*p)(char)) {
  while(1) {
    if (iter->current == iter->str->bytes - 1) goto eof;

    if (iter->prev && iter->current >= iter->prev->bytes && iter->current < iter->prev->bytes + iter->prev->len) {
      // iter->current points to string inside iter->prev
      for(;iter->current >= iter->prev->bytes;iter->current--) {
        if (p(*(iter->current))) goto pred_end;
      }
      iter->current = iter->str->bytes + iter->prev->start - 1;
      iter->next = iter->prev;
      iter->prev = iter->prev->prev;
      continue;
    }

    char *middle_start = iter->prev ? iter->str->bytes + (iter->prev->start + iter->prev->deleted) : iter->str->bytes;
    char *middle_end = iter->next ? iter->str->bytes + iter->next->start : iter->str->bytes + iter->str->len;

    if (iter->current >= middle_start && iter->current < middle_end) {
      // iter->current points to string between iter->prev and iter->next
      for(;iter->current >= middle_start;iter->current--) {
        if (p(*(iter->current))) goto pred_end;
      }
      iter->current = iter->prev ? iter->prev->bytes + iter->prev->len - 1 : middle_start - 1;
      continue;
    }

    if (iter->next && iter->current >= iter->next->bytes && iter->current < iter->next->bytes + iter->next->len) {
      // iter->current points to string inside iter->next
      for(;iter->current < iter->next->bytes + iter->next->len;iter->current--) {
        if (p(*(iter->current))) goto pred_end;
      }
      iter->current = middle_end - 1;
      continue;
    }
  }

  pred_end:
  return true;

  eof:
  buff_string_begin(iter, iter->str);
  return false;
}

void buff_string_takewhile(struct buff_string_iter *iter, char *dest, bool (*p)(char)) {
  int j=0;
  char *end = iter->str->bytes + iter->str->len;
  while(1) {
    if (iter->current == end) goto eof;

    if (iter->prev && iter->current >= iter->prev->bytes && iter->current < iter->prev->bytes + iter->prev->len) {
      // iter->current points to string inside iter->prev
      for(;iter->current < iter->prev->bytes + iter->prev->len;iter->current++) {
        if (!p(*(iter->current))) goto pred_end;
        dest[j++] = *(iter->current);
      }
      iter->current = iter->str->bytes + (iter->prev->start + iter->prev->deleted);
      continue;
    }

    char *middle_start = iter->prev ? iter->str->bytes + (iter->prev->start + iter->prev->deleted) : iter->str->bytes;
    char *middle_end = iter->next ? iter->str->bytes + iter->next->start : iter->str->bytes + iter->str->len;

    if (iter->current >= middle_start && iter->current < middle_end) {
      // iter->current points to string between iter->prev and iter->next
      for(;iter->current < middle_end;iter->current++) {
        if (!p(*(iter->current))) goto pred_end;
        dest[j++] = *(iter->current);
      }
      iter->current = iter->next ? iter->next->bytes : middle_end;
      continue;
    }

    if (iter->next && iter->current >= iter->next->bytes && iter->current < iter->next->bytes + iter->next->len) {
      // iter->current points to string inside iter->next
      for(;iter->current < iter->next->bytes + iter->next->len;iter->current++) {
        if (!p(*(iter->current))) goto pred_end;
        dest[j++] = *(iter->current);
      }
      iter->current = iter->str->bytes + (iter->next->start + iter->next->deleted);
      iter->prev = iter->next;
      iter->next = iter->next->next;
      continue;
    }
  }

  pred_end:
  dest[j]='\0';
  return;

  eof:
  iter->current = NULL;
  iter->prev = NULL;
  iter->next = iter->str->first;
  dest[j]='\0';
  return;
}

void buff_string_begin(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->first && str->first->start==0 ? str->first->bytes : str->bytes;
  iter->next = str->first;
  iter->prev = NULL;
}

void buff_string_end(struct buff_string_iter *iter, struct buff_string *str) {
  iter->str=str;
  iter->current = str->last && str->last->start + str->last->deleted==str->len ? str->last->bytes + str->last->len - 1 : str->bytes + str->len - 1;
  iter->next = NULL;
  iter->prev = str->last;
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

  while (1) {
    if (iter0.current == iter0.str->bytes - 1) return offset - 1;

    if (iter0.prev && iter0.current >= iter0.prev->bytes && iter0.current < iter0.prev->bytes + iter0.prev->len) {
      // iter0.current points to string inside iter0.prev
      offset += iter0.current - iter0.prev->bytes + 1;
      iter0.current = iter0.str->bytes + iter0.prev->start - 1;
      iter0.next = iter0.prev;
      iter0.prev = iter0.prev->prev;
      continue;
    }

    char *middle_start = iter0.prev ? iter0.str->bytes + (iter0.prev->start + iter0.prev->deleted) : iter0.str->bytes;
    char *middle_end = iter0.next ? iter0.str->bytes + iter0.next->start : iter0.str->bytes + iter0.str->len;

    if (iter0.current >= middle_start && iter0.current < middle_end) {
      // iter0.current points to string between iter0.prev and iter0.next
      offset += iter0.current - middle_start + 1;
      iter0.current = iter0.prev ? iter0.prev->bytes + iter0.prev->len - 1 : middle_start - 1;
      continue;
    }

    if (iter0.next && iter0.current >= iter0.next->bytes && iter0.current < iter0.next->bytes + iter0.next->len) {
      // iter0.current points to string inside iter0.next
      offset += iter0.current - iter0.next->bytes + 1;
      iter0.current = middle_end - 1;
      continue;
    }
  }
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
  struct buff_string str02 = {s01, strlen(s01), splice02, NULL};

  buff_string_begin(&iter, &str02);

  printf("iter.current=%lu\n", (uintptr_t)iter.current);
  printf("iter.prev=%lu\n", (uintptr_t)iter.prev);
  printf("iter.next=%lu\n", (uintptr_t)iter.next);
  if (iter.next) {
    printf("iter.next->bytes=%lu\n", (uintptr_t)iter.next->bytes);
    printf("iter.next->start=%lu\n", (uintptr_t)iter.next->start);
    printf("iter.next->len=%lu\n", (uintptr_t)iter.next->len);
  }

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
  printf("iter.current=%p\n", iter.current);
  int offset = buff_string_offset(&iter);
  printf("offset=%d\n", offset);
  assert(offset==30);

  free_buff_string(&str01);
  free_buff_string(&str02);
}
