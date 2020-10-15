#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "cursor.h"

struct loaded_font {
  TTF_Font *font;
  int font_size;
  int X_width;
  int X_height;
};

struct selection {
  bool active;
  struct buff_string_iter mark1;
  struct buff_string_iter mark2;
};

struct buffer {
  struct buff_string contents;
  struct scroll scroll;
  struct cursor cursor;
  struct loaded_font font;
  struct selection selection;
  SDL_Point size;
  int fd;
  bool _last_command;
  SDL_Keysym _prev_keysym;
};

void buffer_init(struct buffer *out, SDL_Point *size, char *path);
void buffer_destroy(struct buffer *self);
bool buffer_update(struct buffer *self, SDL_Event *e);
void buffer_view(struct buffer *self, SDL_Renderer *renderer);
