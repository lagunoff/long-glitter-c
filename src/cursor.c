#include <ctype.h>
#include "cursor.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void cursor_up(struct cursor *c, char *buf) {
  int i = c->pos - 1;
  int n = 0;
  for (; i>0; i--) {
    if (buf[i] == '\n' && n >= 1) break;
    if (buf[i] == '\n') n++;
  }
  c->pos = i == 0 ? i : i + 1;
}

void cursor_down(struct cursor *c, char *buf, int buf_len) {
  int i = c->pos;
  for (; i<buf_len; i++) {
    if (buf[i] == '\n') break;
  }
  c->pos = i + 1;
}

void cursor_left(struct cursor *c) {
  c->pos = MAX(c->pos - 1, 0);
}

void cursor_right(struct cursor *c, int buf_len) {
  c->pos = MIN(c->pos + 1, buf_len);
}

void cursor_bol(struct cursor *c, char *buf) {
  int i = buf[c->pos] == '\n' ? c->pos - 1 : c->pos;
  for (; i>0; i--) {
    if (buf[i] == '\n') break;
  }
  c->pos = i == 0 ? i : i + 1;
}

void cursor_eol(struct cursor *c, char *buf, int buf_len) {
  int i = c->pos;
  for (; i < buf_len;i++) {
    if (buf[i] == '\n') break;
  }
  c->pos = i;
}

void cursor_backward_word(struct cursor *c, char *buf) {
  int i = c->pos;
  for (; i>0; i--) {
    if (isalnum(buf[i])) break;
  }
  for (; i>0; i--) {
    if (!isalnum(buf[i])) break;
  }
  c->pos = i;
}

void cursor_forward_word(struct cursor *c, char *buf, int buf_len) {
  int i = c->pos;
  for (; i < buf_len;i++) {
    if (isalnum(buf[i])) break;
  }
  for (; i < buf_len;i++) {
    if (!isalnum(buf[i])) break;
  }
  c->pos = i;
}

void scroll_up(struct scroll *s, char *buf, int n) {
  int i = s->pos - 1;
  for (int j=0; j<n; j++) {
    int newlines = 0;
    for (; i>0; i--) {
      if (buf[i] == '\n' && newlines >= 1) break;
      if (buf[i] == '\n') newlines++;
    }
  }
  s->pos = i == 0 ? i : i + 1;
}

void scroll_down(struct scroll *s, char *buf, int buf_len, int n) {
  int i = s->pos;
  for (int j=0; j<n; j++) {
    for (; i<buf_len; i++) {
      if (buf[i] == '\n') { i++; break; }
    }
  }
  s->pos = i;
}
