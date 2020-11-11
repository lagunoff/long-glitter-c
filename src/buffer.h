#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "draw.h"
#include "widget.h"
#include "menulist.h"
#include "statusbar.h"
#include "c-mode.h"
#include "input.h"

typedef struct {
  SDL_Window   *window;
  SDL_Renderer *renderer;
} command_palette_t;

typedef struct {
  SDL_Rect textarea;
  SDL_Rect statusbar;
  SDL_Rect lines;
} buffer_geometry_t;

struct buffer_t {
  draw_context_t  ctx;
  char           *path;
  int             fd;
  buffer_geometry_t geometry;
  bool            show_lines;
  // Children widgets
  input_t         input;
  menulist_t      context_menu;
  statusbar_t     statusbar;
};
typedef struct buffer_t buffer_t;

typedef enum {
  BUFFER_CONTEXT_MENU = MSG_USER,
  BUFFER_INPUT,
} buffer_msg_tag_t;

typedef union {
  menulist_item_t menulist_item;
  struct {
    char *title;
    bool  disabled;
    int   icon;
    buffer_msg_tag_t action;
  };
} buffer_context_menu_item_t __attribute__((__transparent_union__));

typedef union {
  widget_msg_t widget;
  struct {
    buffer_msg_tag_t tag;
    union {
      menulist_msg_t context_menu;
      input_msg_t    input;
    };
  };
} buffer_msg_t;

void buffer_init(buffer_t *self, char *path, int font_size);
void buffer_free(buffer_t *self);
void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield);

void buffer_dispatch_sdl(buffer_t *self, buffer_msg_t *msg, yield_t yield, yield_t yield_cm);
bool buffer_iter_screen_xy(buffer_t *self, buff_string_iter_t *iter, int x, int y, bool x_adjust);
void buffer_get_geometry(buffer_t *self, buffer_geometry_t *geometry);
