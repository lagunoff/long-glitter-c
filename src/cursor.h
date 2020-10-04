
struct cursor {
  int x0;
  int pos;
};

struct scroll {
  int pos;
};

void cursor_up(struct cursor *c, char *buf);
void cursor_down(struct cursor *c, char *buf, int buf_len);
void cursor_left(struct cursor *c);
void cursor_right(struct cursor *c, int buf_len);
void cursor_bol(struct cursor *c, char *buf);
void cursor_eol(struct cursor *c, char *buf, int buf_len);
void cursor_backward_word(struct cursor *c, char *buf);
void cursor_forward_word(struct cursor *c, char *buf, int buf_len);

void scroll_up(struct scroll *s, char *buf, int n);
void scroll_down(struct scroll *s, char *buf, int buf_len, int n);
