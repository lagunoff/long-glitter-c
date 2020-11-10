#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "draw.h"
#include "widget.h"
#include "menulist.h"
#include "c-mode.h"

typedef enum {
  SELECTION_INACTIVE,
  SELECTION_DRAGGING_MOUSE,
  SELECTION_DRAGGING_KEYBOARD,
  SELECTION_COMPLETE,
} selection_state_t;

typedef struct {
  selection_state_t  state;
  buff_string_iter_t mark_1;
  buff_string_iter_t mark_2;
} selection_t;

typedef void (*highlighter_cb_t)(char *input, int len, draw_context_t *ctx);

typedef struct {
  draw_context_t *ctx;
  char           *input;
  int             len;
} highlighter_args_t;

typedef struct {
  void (*reset)(void *self);
  void (*forward)(void *self, char *input, int len);
  void (*highlight)(void *self, highlighter_args_t *args, highlighter_cb_t cb);
} highlighter_t;

typedef struct {
  draw_context_t ctx;
  buff_string_t *contents;
  draw_font_t    font;
  scroll_t       scroll;
  cursor_t       cursor;
  selection_t    selection;
  menulist_t     context_menu;
  SDL_Cursor*    ibeam_cursor;
  int           *lines;
  int            lines_len;
  highlighter_t *highlighter;
  char           highlighter_inst[16];
  bool           _last_command;
  SDL_Keysym     _prev_keysym;
} input_t;

typedef enum {
  INPUT_CONTEXT_MENU = MSG_USER,
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
  struct {
    input_msg_tag_t tag;
    union {
      menulist_msg_t context_menu;
    };
  };
} input_msg_t;

void input_init(input_t *self, buff_string_t *content);
void input_free(input_t *self);
void input_dispatch(input_t *self, input_msg_t *msg, yield_t yield);

void input_dispatch_sdl(input_t *self, input_msg_t *msg, yield_t yield, yield_t yield_cm);
bool input_iter_screen_xy(input_t *self, buff_string_iter_t *iter, int x, int y, bool x_adjust);
SDL_Point selection_get_range(selection_t *self, cursor_t *cursor);
