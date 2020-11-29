#pragma once

#include "graphics.h"
#include "buff-string.h"

typedef struct {
  buff_string_iter_t pos;
  int                x0;
} cursor_t;

typedef struct {
  buff_string_iter_t pos;
  int                line;
} scroll_t;

void cursor_up(cursor_t *c);
void cursor_down(cursor_t *c);
void cursor_left(cursor_t *c);
void cursor_right(cursor_t *c);
void cursor_bol(cursor_t *c);
void cursor_eol(cursor_t *c);
void cursor_end(cursor_t *c);
void cursor_begin(cursor_t *c);
void cursor_backward_word(cursor_t *c);
void cursor_forward_word(cursor_t *c);
void scroll_lines(scroll_t *s, int n);
void backward_paragraph(cursor_t *c);
void forward_paragraph(cursor_t *c);
void scroll_page(widget_t *self, font_t *font, scroll_t *s, cursor_t *c, int n);
