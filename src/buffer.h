#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>
#include <cairo.h>

#include "buff-string.h"
#include "cursor.h"
#include "cursor.h"

struct selection {
  bool active;
  buff_string_iter_t mark1;
  buff_string_iter_t mark2;
};

struct buffer {
  buff_string_t contents;
  struct scroll scroll;
  struct cursor cursor;
  struct selection selection;
  SDL_Point size;
  cairo_font_extents_t fe;
  bool fe_initialized;
  int fd;
  int font_size;
  bool _last_command;
  SDL_Keysym _prev_keysym;
};

void buffer_init(struct buffer *out, SDL_Point *size, char *path);
void buffer_destroy(struct buffer *self);
bool buffer_update(struct buffer *self, SDL_Event *e);
void buffer_view(struct buffer *self, cairo_t *cr);
