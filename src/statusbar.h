#pragma once

#include <SDL2/SDL.h>

#include "buffer.h"
#include "cursor.h"
#include "cursor.h"

typedef struct {
  struct loaded_font *font;
  struct cursor *cursor;
} statusbar_t;

bool statusbar_update(statusbar_t *self, SDL_Event *e);
void statusbar_view(statusbar_t *self, SDL_Renderer *renderer);
