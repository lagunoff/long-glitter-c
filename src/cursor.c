#include <ctype.h>
#include "cursor.h"
#include "main.h"
#include "buff-string.h"
#include "buffer.h"

static void cursor_fixup_x0(struct cursor *cur) {
  struct buff_string_iter iter = cur->pos;
  cur->x0 = 0;
  buff_string_find_back(&iter, lambda(bool _(char c) {
    if (c!='\n') cur->x0++;
    return c=='\n';
  }));
}

void cursor_up(struct cursor *c) {
  struct buff_string_iter i0 = c->pos;
  bool eof0 = buff_string_iterate(&i0, BSD_LEFT, BSLIP_DO, lambda(bool _(char c) {
    return c=='\n';
  }));

  if (eof0) return;

  struct buff_string_iter i1 = i0;
  int line_len = 0;
  bool eof = buff_string_iterate(&i1, BSD_LEFT, BSLIP_DONT, lambda(bool _(char c) {
    if (c!='\n') line_len++;
    return c=='\n';
  }));
  buff_string_move(&i1, MIN(line_len, c->x0));
  c->pos = i1;
}

void cursor_down(struct cursor *c) {
  struct buff_string_iter i0 = c->pos;
  if (*(c->pos.current)!='\n') buff_string_move(&i0, 1);
  bool eof0 = buff_string_find(&i0, lambda(bool _(char c) {
    return c=='\n';
  }));
  if (eof0) return;

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
  bool eof = buff_string_find_back(&(c->pos), lambda(bool _(char c) {
    return c=='\n';
  }));
  if (!eof) {
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
    bool eof = buff_string_find_back(&(s->pos), lambda(bool _(char c) {
      return c=='\n';
    }));
    if (!eof) buff_string_move(&(s->pos), 1);
  } else {
    int newlines = 0;
    buff_string_find_back(&(s->pos), lambda(bool _(char c) {
      if (c=='\n') {
        if (!(newlines++ < abs(n))) return true;
      }
      return false;
    }));
    bool eof = buff_string_find_back(&(s->pos), lambda(bool _(char c) {
      return c=='\n';
    }));
    if (!eof) buff_string_move(&(s->pos), 1);
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

  bool eof = buff_string_find_back(&i0, lambda(bool _(char c) {
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
  if (eof) buff_string_begin(&(cur->pos), cur->pos.str);
  cursor_fixup_x0(cur);
}

void forward_paragraph(struct cursor *cur) {
  int is_nl = false;
  struct buff_string_iter i0 = cur->pos;
  bool eof = buff_string_find(&i0, lambda(bool _(char c) {
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
  if (eof) buff_string_end(&(cur->pos), cur->pos.str);
  cursor_fixup_x0(cur);
}

void cursor_unittest() {
  printf("cursor_unittest\n");
}
