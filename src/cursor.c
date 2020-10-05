#include <ctype.h>
#include "cursor.h"
#include "main.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define SCROLL_JUMP 8

static void cursor_fixup_x0(struct cursor *c, char *buf) {
  int i = c->pos;
  for (;i>0;i--) {
    if (buf[i - 1] == '\n') break;
  }
  c->x0 = c->pos - i;
}

void cursor_up(struct cursor *c, char *buf) {
  int i0 = c->pos;
  for (; i0>0; i0--) {
    if (buf[i0 - 1] == '\n') break;
  }
  int i1 = i0 - 1;
  for (; i1>0; i1--) {
    if (buf[i1 - 1] == '\n') break;
  }
  c->pos = MAX(0, MIN(i0 - 1, (i1 + c->x0)));
}

void cursor_down(struct cursor *c, char *buf, int buf_len) {
  int i0 = c->pos;
  for (; i0<buf_len; i0++) {
    if (buf[i0] == '\n') break;
  }
  int i1 = i0 + 1;
  for (; i1<buf_len; i1++) {
    if (buf[i1] == '\n') break;
  }
  c->pos = MIN(MIN(i1, (i0 + 1 + c->x0)), buf_len);
}

void cursor_left(struct cursor *c, char *buf) {
  c->pos = MAX(c->pos - 1, 0);
  cursor_fixup_x0(c, buf);
}

void cursor_right(struct cursor *c, char *buf, int buf_len) {
  c->pos = MIN(c->pos + 1, buf_len);
  cursor_fixup_x0(c, buf);
}

void cursor_bol(struct cursor *c, char *buf) {
  int i = buf[c->pos] == '\n' ? c->pos - 1 : c->pos;
  for (; i>0; i--) {
    if (buf[i] == '\n') break;
  }
  c->pos = i == 0 ? i : i + 1;
  cursor_fixup_x0(c, buf);
}

void cursor_eol(struct cursor *c, char *buf, int buf_len) {
  int i = c->pos;
  for (; i < buf_len;i++) {
    if (buf[i] == '\n') break;
  }
  c->pos = i;
  cursor_fixup_x0(c, buf);
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
  cursor_fixup_x0(c, buf);
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
  cursor_fixup_x0(c, buf);
}

void scroll_lines(struct scroll *s, char *buf, int buf_len, int n) {
  printf("scroll_lines: %d %d\n", s->pos, n);
  if (n > 0) {
    int i = s->pos;
    for (int j=0; j<n; j++) {
      for (; i<buf_len; i++) {
        if (buf[i] == '\n') { i++; break; }
      }
    }
    s->pos = i;
  } else {
    int i = s->pos;
    int newlines = 0;
    for (; i>0; i--) {
      if (buf[i] == '\n') newlines++;
      if (newlines > abs(n) + 1) break;
    }
    s->pos = i;
  }
}

void cursor_modified (
  struct scroll *s,
  struct cursor *prev,
  struct cursor *next,
  struct loaded_font *lf,
  int height,
  char *buf,
  int buf_len
) {
  int diff = next->pos - prev->pos;
  char *cur_ptr = buf + next->pos;
  char *scroll_ptr = buf + s->pos;
  int screen_lines = div(height, lf->X_height).quot;

  int count_lines() {
    int lines=0;
    for (char *i=cur_ptr; i!=scroll_ptr;) {
      if (cur_ptr > scroll_ptr) {
        if (*i=='\n') lines++;
        i--;
      } else {
        if (*i=='\n') lines--;
        i++;
      }
    }
    return lines;
  }
  int lines = count_lines();

  printf("screen_lines %d\n", screen_lines);
  printf("lines %d\n", lines);
  printf("next->pos %d\n", next->pos);

  if (diff > 0) {
    if (lines > screen_lines) {
      scroll_lines(s, buf, buf_len, lines - screen_lines + SCROLL_JUMP);
    }
  } else {
    int lines = count_lines();
    if (lines < 0) {
      scroll_lines(s, buf, buf_len, lines - SCROLL_JUMP - 1);
    }
  }
}

void scroll_page(
  struct scroll *s,
  struct cursor *c,
  struct loaded_font *lf,
  int height,
  char *buf,
  int buf_len,
  int n
) {
  int screen_lines = div(height, lf->X_height).quot;
  if (n > 0) {
    scroll_lines(s, buf, buf_len, screen_lines);
  } else {
    scroll_lines(s, buf, buf_len, -screen_lines);
  }
  c->pos = s->pos;
  cursor_fixup_x0(c, buf);
}

void backward_paragraph(struct cursor *c, char *buf) {
  int i0 = c->pos;
  for (; i0>0; i0--) if (!isspace(buf[i0])) break;
  for (; i0>0; i0--) {
    if (buf[i0] == '\n') {
      for (int i1=i0 - 1; i1>0; i1--) {
        if (buf[i1] == '\n') {
          goto found_paragraph;
        }
        if (!isspace(buf[i1])) break;
      }
    }
  }
  found_paragraph:
  c->pos = i0;
  cursor_fixup_x0(c, buf);
}

void forward_paragraph(struct cursor *c, char *buf, int buf_len) {
  int i0 = c->pos;
  for (; i0<buf_len; i0++) {
    if (buf[i0] == '\n') {
      for (int i1=i0 + 1; i1<buf_len; i1++) {
        if (buf[i1] == '\n') {
          i0 = i1;
          goto found_paragraph;
        }
        if (!isspace(buf[i1])) break;
      }
    }
  }
  found_paragraph:
  c->pos = i0;
  cursor_fixup_x0(c, buf);
}
