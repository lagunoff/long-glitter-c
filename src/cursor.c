#include <ctype.h>
#include "cursor.h"
#include "main.h"
#include "buff-string.h"

#define SCROLL_JUMP 8

static void cursor_fixup_x0(struct cursor *cur) {
  struct buff_string_iter iter = cur->pos;
  bool no_eof = buff_string_move(&iter, -1);
  cur->x0 = 0;
  if (no_eof) buff_string_find_back(&iter, lambda(bool _(char c) {
    if (c!='\n') cur->x0++;
    return c=='\n';
  }));
}

void cursor_up(struct cursor *c) {
  struct buff_string_iter i0 = c->pos;
  buff_string_move(&i0, -1);
  bool found0 = buff_string_find_back(&i0, lambda(bool _(char c) {
    return c=='\n';
  }));

  if (!found0) return;

  struct buff_string_iter i1 = i0;
  int line_len = 0;
  buff_string_move(&i1, -1);
  bool found = buff_string_find_back(&i1, lambda(bool _(char c) {
    if (c!='\n') line_len++;
    return c=='\n';
  }));
  if (found) {
    buff_string_move(&i1, MIN(line_len + 1, c->x0 + 1));
  } else {
    buff_string_move(&i1, MIN(line_len, c->x0));
  }
  c->pos = i1;
}

void cursor_down(struct cursor *c) {
  struct buff_string_iter i0 = c->pos;
  if (*(c->pos.current)!='\n') buff_string_move(&i0, 1);
  bool found0 = buff_string_find(&i0, lambda(bool _(char c) {
    return c=='\n';
  }));
  if (!found0) return;

  struct buff_string_iter i1 = i0;
  int line_len = 0;
  buff_string_move(&i1, 1);
  buff_string_find(&i1, lambda(bool _(char c) {
    if (c!='\n') line_len++;
    return c=='\n';
  }));
  buff_string_move(&i0, MIN(line_len + 1, c->x0 + 1));
  c->pos = i0;
}

void cursor_left(struct cursor *c) {
  buff_string_move(&(c->pos), -1);
  cursor_fixup_x0(c);
}

void cursor_right(struct cursor *c) {
  buff_string_move(&(c->pos), 1);
  cursor_fixup_x0(c);
}

void cursor_bol(struct cursor *c) {
  if (*(c->pos.current)=='\n') {
    buff_string_move(&(c->pos), -1);
  }
  bool found = buff_string_find_back(&(c->pos), lambda(bool _(char c) {
    return c=='\n';
  }));
  if (found) {
    buff_string_move(&(c->pos), 1);
  }
  cursor_fixup_x0(c);
}

void cursor_eol(struct cursor *c) {
  buff_string_find(&(c->pos), lambda(bool _(char c) {
    return c=='\n';
  }));
  cursor_fixup_x0(c);
}

void cursor_backward_word(struct cursor *c) {
  buff_string_backward_word(&c->pos);
  cursor_fixup_x0(c);
}

void cursor_forward_word(struct cursor *c) {
  buff_string_forward_word(&c->pos);
  cursor_fixup_x0(c);
}

void cursor_end(struct cursor *c) {
  buff_string_end(&(c->pos), c->pos.str);
  cursor_fixup_x0(c);
}

void cursor_begin(struct cursor *c) {
  buff_string_begin(&(c->pos), c->pos.str);
  cursor_fixup_x0(c);
}

void scroll_lines(struct scroll *s, int n) {
  if (n > 0) {
    int newlines = 0;
    buff_string_find(&(s->pos), lambda(bool _(char c) {
      if (c=='\n') {
        if (!(newlines++ < n)) return true;
      }
      return false;
    }));
    bool found = buff_string_find_back(&(s->pos), lambda(bool _(char c) {
      return c=='\n';
    }));
    if (found) buff_string_move(&(s->pos), 1);
  } else {
    int newlines = 0;
    buff_string_find_back(&(s->pos), lambda(bool _(char c) {
      if (c=='\n') {
        if (!(newlines++ < abs(n))) return true;
      }
      return false;
    }));
    bool found = buff_string_find_back(&(s->pos), lambda(bool _(char c) {
      return c=='\n';
    }));
    if (found) buff_string_move(&(s->pos), 1);
  }
}

void cursor_modified (
  struct scroll *s,
  struct cursor *prev,
  struct cursor *next,
  struct loaded_font *lf,
  int height
) {
  int next_offset = buff_string_offset(&(next->pos));
  int prev_offset = buff_string_offset(&(prev->pos));
  int scroll_offset = buff_string_offset(&(s->pos));
  int diff = next_offset - prev_offset;
  int screen_lines = div(height, lf->X_height).quot;

  int count_lines() {
    struct buff_string_iter iter = next->pos;
    int lines=0;
    int i = next_offset;
    if (next_offset > scroll_offset) {
      buff_string_find_back(&iter, lambda(bool _(char c) {
        if (c=='\n') lines++;
        i--;
        if (i==scroll_offset) return true;
        return false;
      }));
    } else {
      buff_string_find(&iter, lambda(bool _(char c) {
        if (c=='\n') lines--;
        i++;
        if (i==scroll_offset) return true;
        return false;
      }));
    }
    return lines;
  }
  int lines = count_lines();

  if (diff > 0) {
    if (lines > screen_lines) {
      scroll_lines(s, lines - screen_lines + SCROLL_JUMP);
    }
  } else {
    if (lines < 0) {
      scroll_lines(s, lines - SCROLL_JUMP - 1);
    }
  }
}

void scroll_page(
  struct scroll *s,
  struct cursor *c,
  struct loaded_font *lf,
  int height,
  int n
) {
  int screen_lines = div(height, lf->X_height).quot;
  if (n > 0) {
    scroll_lines(s, screen_lines);
  } else {
    scroll_lines(s, -screen_lines);
  }
  c->pos = s->pos;
  cursor_fixup_x0(c);
}

void backward_paragraph(struct cursor *cur) {
  int is_nl = false;
  struct buff_string_iter i0 = cur->pos;
  buff_string_find_back(&i0, lambda(bool _(char c) {
    return !isspace(c);
  }));

  bool found = buff_string_find_back(&i0, lambda(bool _(char c) {
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
  if (!found) buff_string_begin(&(cur->pos), cur->pos.str);
  cursor_fixup_x0(cur);
}

void forward_paragraph(struct cursor *cur) {
  int is_nl = false;
  struct buff_string_iter i0 = cur->pos;
  bool found = buff_string_find(&i0, lambda(bool _(char c) {
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
  if (!found) buff_string_end(&(cur->pos), cur->pos.str);
  cursor_fixup_x0(cur);
}

void cursor_unittest() {
  printf("cursor_unittest\n");
}
