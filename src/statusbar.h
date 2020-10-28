#pragma once

#include <SDL2/SDL.h>

#include "cursor.h"
#include "draw.h"

struct buffer_t;

typedef struct {
  draw_context_t   ctx;
  struct buffer_t *buffer;
} statusbar_t;

void statusbar_init(statusbar_t *self , struct buffer_t *buffer);
void statusbar_measure(statusbar_t *self, SDL_Point *size);
bool statusbar_update(statusbar_t *self, SDL_Event *e);
void statusbar_view(statusbar_t *self);
