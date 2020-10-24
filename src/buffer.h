#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <cairo.h>

#include "buff-string.h"
#include "cursor.h"
#include "cursor.h"

typedef struct {
  bool               active;
  buff_string_iter_t mark1;
  buff_string_iter_t mark2;
} selection_t;

typedef struct {
  buff_string_t contents;
  scroll_t      scroll;
  cursor_t      cursor;
  selection_t   selection;
  SDL_Point     size;
  cairo_font_extents_t fe;
  bool          fe_initialized;
  int           fd;
  int           font_size;
  bool          _last_command;
  SDL_Keysym    _prev_keysym;
} buffer_t;

void buffer_init(buffer_t *out, SDL_Point *size, char *path);
void buffer_destroy(buffer_t *self);
bool buffer_update(buffer_t *self, SDL_Event *e);
void buffer_view(buffer_t *self, cairo_t *cr);
