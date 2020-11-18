#pragma once
#include <stdbool.h>
#include <X11/Xcursor/Xcursor.h>

#include "buff-string.h"
#include "cursor.h"
#include "draw.h"
#include "widget.h"
#include "menulist.h"
#include "utils.h"

#define MODIFY_CURSOR(f) {                                   \
  cursor_t old_cursor = self->cursor;                        \
  f(&self->cursor);                                               \
  cursor_modified(self, &old_cursor);                        \
}

typedef enum {
  SELECTION_INACTIVE,
  SELECTION_DRAGGING_MOUSE,
  SELECTION_DRAGGING_KEYBOARD,
  SELECTION_COMPLETE,
} selection_state_t;

typedef struct {
  syntax_style_t syntax;
  bool           selected;
} text_style_t;

typedef enum {
  CURSOR_LINE,
  CURSOR_BLOCK,
} cursor_style_t;

typedef struct {
  selection_state_t  state;
  buff_string_iter_t mark_1;
  buff_string_iter_t mark_2;
} selection_t;

typedef void (*highlighter_t)(point_t range, text_style_t *style);
typedef void (*highlighter_inv_t)(highlighter_t highlighter);
typedef void (*style_on_t)(text_style_t *style);
typedef void (*style_off_t)(text_style_t *style);

typedef struct {
  widget_context_t *ctx;
  text_style_t   *normal;
  char           *input;
  int             len;
} highlighter_args_t;

typedef struct {
  void (*reset)(void *self);
  void (*forward)(void *self, char *input, int len);
  void (*highlight)(void *self, highlighter_args_t *args, highlighter_t hl);
} syntax_highlighter_t;

typedef struct {
  widget_context_t ctx;
  buff_string_t *contents;
  XftFont       *font;
  cursor_style_t cursor_style;
  scroll_t       scroll;
  cursor_t       cursor;
  selection_t    selection;
  menulist_t     context_menu;
  //! Offsets of the line beginnings, -1 if the line is outside text
  //! contents
  int           *lines;
  int            lines_len;
  syntax_highlighter_t *syntax_hl;
  char           syntax_hl_inst[16];
  Cursor         x_cursor;
  char          *x_selection;
} input_t;

typedef enum {
  INPUT_CONTEXT_MENU = MSG_LAST,
  INPUT_CUT,
  INPUT_COPY,
  INPUT_PASTE,
} input_msg_tag_t;

typedef union {
  menulist_item_t menulist_item;
  struct {
    char *title;
    bool  disabled;
    int   icon;
    input_msg_tag_t action;
  };
} input_context_menu_item_t __attribute__((__transparent_union__));

typedef union {
  widget_msg_t widget;
  XEvent       x_event;
  struct {
    input_msg_tag_t tag;
    union {
      menulist_msg_t context_menu;
    };
  };
} input_msg_t;

void input_init(input_t *self, widget_context_init_t *ctx, buff_string_t *content);
void input_free(input_t *self);
void input_dispatch(input_t *self, input_msg_t *msg, yield_t yield);
bool input_iter_screen_xy(input_t *self, buff_string_iter_t *iter, int x, int y, bool x_adjust);
void input_set_style(widget_context_t *ctx, text_style_t *style);
point_t selection_get_range(selection_t *self, cursor_t *cursor);

syntax_highlighter_t noop_highlighter;

__inline__ __attribute__((always_inline)) void
input_highlight_range(style_on_t on, style_off_t off, point_t range, highlighter_inv_t inv_hl, highlighter_t hl) {
  inv_hl(lambda(void _(point_t r, text_style_t *s) {
    if (r.x >= range.x) {
      if (r.y <= range.y) {
        on(s); hl(r, s);
      } else {
        point_t p1; point_t p2;
        p1.x = r.x; p1.y = range.x;
        p2.x = range.x; p2.y = r.y;
        on(s); hl(p1, s);
        off(s); hl(p2, s);
      }
    } else {
      if (r.y <= range.x) {
        off(s); hl(r, s);
      } else {
        point_t p1; point_t p2;
        p1.x = r.x; p1.y = range.x;
        p2.x = range.x; p2.y = r.y;
        off(s); hl(p1, s);
        on(s); hl(p2, s);
      }
    }
    return;
  }));
}
