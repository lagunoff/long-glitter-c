#include <ctype.h>

#include "cursor.h"
#include "utils.h"
#include "buff-string.h"
#include "buffer.h"

static void cursor_fixup_x0(cursor_t *cur) {
  buff_string_iter_t iter = cur->pos;
  cur->x0 = 0;
  bs_find_back(&iter, lambda(bool _(char c) {
    if (c!='\n') cur->x0++;
    return c=='\n';
  }));
}

void cursor_up(cursor_t *c) {
  buff_string_iter_t i0 = c->pos;
  bool eof0 = bs_iterate(&i0, BS_LEFT, BS_DO_INCREMENT, lambda(bool _(char c) {
    return c=='\n';
  }));

  if (eof0) return;

  buff_string_iter_t i1 = i0;
  int line_len = 0;
  bs_iterate(&i1, BS_LEFT, BS_DONT_INCREMENT, lambda(bool _(char c) {
    if (c!='\n') line_len++;
    return c=='\n';
  }));
  bs_move(&i1, MIN(line_len, c->x0));
  c->pos = i1;
}

void cursor_down(cursor_t *c) {
  buff_string_iter_t i0 = c->pos;
  bool eof0 = bs_iterate(&i0, BS_RIGHT, BS_DO_INCREMENT, lambda(bool _(char c) {
    return c=='\n';
  }));
  if (eof0) return;

  buff_string_iter_t i1 = i0;
  int line_len = 0;
  bs_iterate(&i1, BS_RIGHT, BS_DONT_INCREMENT, lambda(bool _(char c) {
    if (c!='\n') line_len++;
    return c=='\n';
  }));
  bs_move(&i0, MIN(line_len, c->x0));
  c->pos = i0;
}

void cursor_left(cursor_t *c) {
  bs_move(&(c->pos), -1);
  cursor_fixup_x0(c);
}

void cursor_right(cursor_t *c) {
  bs_move(&(c->pos), 1);
  cursor_fixup_x0(c);
}

void cursor_bol(cursor_t *c) {
  bool eof = bs_iterate(&(c->pos), BS_LEFT, BS_DONT_INCREMENT, lambda(bool _(char c) {
    return c=='\n';
  }));
  cursor_fixup_x0(c);
}

void cursor_eol(cursor_t *c) {
  bs_find(&(c->pos), lambda(bool _(char c) {
    return c=='\n';
  }));
  cursor_fixup_x0(c);
}

void cursor_backward_word(cursor_t *c) {
  bs_backward_word(&c->pos);
  cursor_fixup_x0(c);
}

void cursor_forward_word(cursor_t *c) {
  bs_forward_word(&c->pos);
  cursor_fixup_x0(c);
}

void cursor_end(cursor_t *c) {
  bs_end(&(c->pos), c->pos.str);
  cursor_fixup_x0(c);
}

void cursor_begin(cursor_t *c) {
  bs_begin(&(c->pos), c->pos.str);
  cursor_fixup_x0(c);
}

void scroll_lines(scroll_t *s, int n) {
  if (n == 0) return;
  if (n > 0) {
    int newlines = 0;
    bool eof = bs_iterate(&s->pos, BS_RIGHT, BS_DO_INCREMENT, lambda(bool _(char c) {
      if (c=='\n') {
        if (!(++newlines < n)) return true;
      }
      return false;
    }));
    s->line += newlines;
  } else {
    int newlines = -1;
    bool eof = bs_iterate(&s->pos, BS_LEFT, BS_DONT_INCREMENT, lambda(bool _(char c) {
      if (c=='\n') {
        if (!(++newlines < abs(n))) return true;
      }
      return false;
    }));
    s->line -= eof ? newlines + 1: newlines;
  }
}

void scroll_page(draw_context_t *ctx, scroll_t *s, cursor_t *c, int n) {
  int screen_lines = div(ctx->clip.h, ctx->font->height).quot;
  if (n > 0) {
    scroll_lines(s, screen_lines);
  } else {
    scroll_lines(s, -screen_lines);
  }
  c->pos = s->pos;
  cursor_fixup_x0(c);
}

void backward_paragraph(cursor_t *cur) {
  int is_nl = false;
  buff_string_iter_t i0 = cur->pos;
  bs_find_back(&i0, lambda(bool _(char c) {
    return !isspace(c);
  }));

  bool eof = bs_find_back(&i0, lambda(bool _(char c) {
    if (is_nl) {
      if (c=='\n') return true;
      if (isspace(c)) return false;
      is_nl=false;
      return false;
    }
    if (c=='\n') {
      cur->pos = i0;
      is_nl=true;
    }
    return false;
  }));
  if (eof) bs_begin(&(cur->pos), cur->pos.str);
  cursor_fixup_x0(cur);
}

void forward_paragraph(cursor_t *cur) {
  int is_nl = false;
  buff_string_iter_t i0 = cur->pos;
  bool eof = bs_find(&i0, lambda(bool _(char c) {
    if (is_nl) {
      if (c=='\n') {
        cur->pos = i0;
        return true;
      }
      if (isspace(c)) return false;
      is_nl=false;
      return false;
    }
    if (c=='\n') {
      is_nl=true;
    }
    return false;
  }));
  if (eof) bs_end(&(cur->pos), cur->pos.str);
  cursor_fixup_x0(cur);
}

void cursor_unittest() {
  printf("cursor_unittest\n");
}
