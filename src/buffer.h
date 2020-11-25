#pragma once
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "graphics.h"
#include "widget.h"
#include "menulist.h"
#include "statusbar.h"
#include "input.h"

typedef struct {
  int    state;
  KeySym keysym;
} keystroke_t;

struct buffer_t {
  widget_context_t ctx;
  char          *path;
  int            fd;
  bool           show_lines;
  font_t         font;
  keystroke_t    modifier;
  // Children widgets
  menulist_t     context_menu;
  input_t        input;
  rect_t         lines;
};
typedef struct buffer_t buffer_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum {
      Buffer_ContextMenu = Widget_Last,
      Buffer_Input,
      Buffer_Save,
    } tag;
    union {
      menulist_msg_t context_menu;
      input_msg_t    input;
    };
  };
} buffer_msg_t;

typedef typeof(((buffer_msg_t *)0)->tag) buffer_msg_tag_t;

typedef union {
  menulist_item_t menulist_item;
  struct {
    char *title;
    bool  disabled;
    int   icon;
    buffer_msg_tag_t action;
  };
} buffer_context_menu_item_t __attribute__((__transparent_union__));

void buffer_init(buffer_t *self, widget_context_init_t *ctx, char *path);
void buffer_free(buffer_t *self);
void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield);
