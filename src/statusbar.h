#pragma once

#include <SDL2/SDL.h>

#include "buffer.h"
#include "cursor.h"
#include "draw.h"

typedef struct {
  buffer_t *buffer;
} statusbar_t;

bool statusbar_update(statusbar_t *self, SDL_Event *e);
void statusbar_view(statusbar_t *self, draw_context_t *ctx);
