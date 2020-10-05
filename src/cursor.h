#pragma once

struct loaded_font;

struct cursor {
  int x0;
  int pos;
};

struct scroll {
  int pos;
};

void cursor_up(struct cursor *c, char *buf);
void cursor_down(struct cursor *c, char *buf, int buf_len);
void cursor_left(struct cursor *c, char *buf);
void cursor_right(struct cursor *c, char *buf, int buf_len);
void cursor_bol(struct cursor *c, char *buf);
void cursor_eol(struct cursor *c, char *buf, int buf_len);
void cursor_backward_word(struct cursor *c, char *buf);
void cursor_forward_word(struct cursor *c, char *buf, int buf_len);
void cursor_forward_word(struct cursor *c, char *buf, int buf_len);
void scroll_lines(struct scroll *s, char *buf, int buf_len, int n);
void backward_paragraph(struct cursor *c, char *buf);
void forward_paragraph(struct cursor *c, char *buf, int buf_len);

void scroll_page(
  struct scroll *s,
  struct cursor *c,
  struct loaded_font *lf,
  int height,
  char *buf,
  int buf_len,
  int n
);

void cursor_modified (
  struct scroll *s,
  struct cursor *prev,
  struct cursor *next,
  struct loaded_font *lf,
  int height,
  char *buf,
  int buf_len
);
