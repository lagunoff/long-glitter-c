#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "draw.h"

typedef bool (*update_t)(void *self, SDL_Event *e);
typedef void (*measure_t)(void *self, SDL_Point *size);
typedef void (*view_t)(void *self);
typedef void (*finalizer_t)(void *self);

struct widget_t {
  update_t    update; //! @null
  measure_t   measure; //! @null
  view_t      view;
  finalizer_t free; //! @null
};
typedef struct widget_t widget_t;

void widget_window_set(SDL_Window *window, const widget_t *widget, const void *model);
void widget_window_get(SDL_Window *window, widget_t **widget, void **model);
