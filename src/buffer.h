#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "buff-string.h"
#include "cursor.h"
#include "draw.h"

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
  buff_string_t *contents;
  draw_font_t    font;
  scroll_t       scroll;
  cursor_t       cursor;
  selection_t    selection;
  SDL_Point      size;
  int            fd;
  char          *path;
  bool           _last_command;
  SDL_Keysym     _prev_keysym;
} buffer_t;

void buffer_init(buffer_t *out, SDL_Point *size, char *path, int font_size);
void buffer_destroy(buffer_t *self);
bool buffer_update(buffer_t *self, SDL_Event *e);
void buffer_view(buffer_t *self, draw_context_t *ctx);
