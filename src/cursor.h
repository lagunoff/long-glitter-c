#pragma once

#include "buff-string.h"

struct loaded_font;

struct cursor {
  struct buff_string_iter pos;
  int x0;
};

struct scroll {
  struct buff_string_iter pos;
};

void cursor_up(struct cursor *c);
void cursor_down(struct cursor *c);
void cursor_left(struct cursor *c);
void cursor_right(struct cursor *c);
void cursor_bol(struct cursor *c);
void cursor_eol(struct cursor *c);
void cursor_end(struct cursor *c);
void cursor_begin(struct cursor *c);
void cursor_backward_word(struct cursor *c);
void cursor_forward_word(struct cursor *c);
void scroll_lines(struct scroll *s, int n);
void backward_paragraph(struct cursor *c);
void forward_paragraph(struct cursor *c);

void scroll_page(
  struct scroll *s,
  struct cursor *c,
  struct loaded_font *lf,
  int height,
  int n
);
