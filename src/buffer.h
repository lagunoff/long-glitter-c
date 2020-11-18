#pragma once
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "draw.h"
#include "widget.h"
#include "menulist.h"
#include "statusbar.h"
#include "input.h"

typedef struct {
  rect_t textarea;
  rect_t statusbar;
} buffer_geometry_t;

struct buffer_t {
  widget_context_t ctx;
  char          *path;
  int            fd;
  bool           show_lines;
  // Children widgets
  menulist_t     context_menu;
  input_t        input;
  statusbar_t    statusbar;
  rect_t         lines;
};
typedef struct buffer_t buffer_t;

typedef union {
  widget_msg_t widget;
  XEvent       x_event;
  struct {
    enum {
      BUFFER_CONTEXT_MENU = MSG_LAST,
      BUFFER_INPUT,
      BUFFER_STATUSBAR,
    } tag;
    union {
      menulist_msg_t context_menu;
      input_msg_t    input;
      statusbar_msg_t statusbar;
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

void buffer_dispatch_sdl(buffer_t *self, buffer_msg_t *msg, yield_t yield, yield_t yield_cm);
bool buffer_iter_screen_xy(buffer_t *self, buff_string_iter_t *iter, int x, int y, bool x_adjust);
void buffer_get_geometry(buffer_t *self, buffer_geometry_t *geometry);
