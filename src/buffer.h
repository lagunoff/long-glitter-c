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

typedef enum {
  BS_INACTIVE,
  BS_ONE_MARK,
  BS_DRAGGING,
  BS_COMPLETE,
} bs_selection_state_t;

typedef struct {
  bs_selection_state_t active;
  buff_string_iter_t   mark1;
  buff_string_iter_t   mark2;
} selection_t;

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
  buff_string_t  *contents;
  char           *path;
  draw_font_t     font;
  scroll_t        scroll;
  cursor_t        cursor;
  selection_t     selection;
  int             fd;
  command_palette_t command_palette;
  menulist_t      context_menu;
  statusbar_t     statusbar;
  buffer_geometry_t geometry;
  int            *lines;
  int             lines_len;
  SDL_Cursor*     ibeam_cursor;
  c_mode_context_t c_mode;
  bool            _last_command;
  SDL_Keysym      _prev_keysym;
};
typedef struct buffer_t buffer_t;

widget_t buffer_widget;
widget_t buffer_context_menu_widget;

void buffer_init(buffer_t *self, char *path, int font_size);
void buffer_destroy(buffer_t *self);
bool buffer_update(buffer_t *self, SDL_Event *e);
void buffer_view(buffer_t *self);
bool buffer_iter_screen_xy(buffer_t *self, buff_string_iter_t *iter, int x, int y, bool x_adjust);
void buffer_get_geometry(buffer_t *self, buffer_geometry_t *geometry);
