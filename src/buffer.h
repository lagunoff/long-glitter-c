#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "draw.h"
#include "widget.h"
#include "menulist.h"

typedef enum {
  BS_INACTIVE,
  BS_ONE_MARK,
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
  draw_context_t ctx;
  draw_font_t    font;
  char         **items;
  int            hover;
} buffer_context_menu_t;

typedef struct {
  buff_string_t  *contents;
  draw_context_t *ctx;
  char           *path;
  draw_font_t     font;
  scroll_t        scroll;
  cursor_t        cursor;
  selection_t     selection;
  int             fd;
  command_palette_t command_palette;
  menulist_t      context_menu;
  bool            _last_command;
  SDL_Keysym      _prev_keysym;
} buffer_t;

widget_t buffer_widget;
widget_t buffer_context_menu_widget;

void buffer_init(buffer_t *self, draw_context_t *ctx, char *path, int font_size);
void buffer_destroy(buffer_t *self);
bool buffer_update(buffer_t *self, SDL_Event *e);
void buffer_view(buffer_t *self);
