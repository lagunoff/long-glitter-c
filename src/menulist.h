#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "widget.h"

typedef struct {
  char *title;
} menulist_item_t;

typedef enum {
  MENULIST_NOOP,
  MENULIST_DESTROY,
} menulist_action_t;

typedef struct {
  draw_context_t   ctx;
  menulist_item_t *items; //! aligned by value of 'alignment' field
  int              len;
  int              alignement; //! sizeof(specific_item_t)
  int              hover;
} menulist_t;

widget_t menulist_widget;

void menulist_init(menulist_t *self, menulist_item_t *items, int len, int alignement);
void menulist_free(menulist_t *self);
void menulist_view(menulist_t *self);
void menulist_measure(menulist_t *self, SDL_Point *size);
bool menulist_update(menulist_t *self, SDL_Event *e);
menulist_action_t menulist_context_update(menulist_t *self, SDL_Event *e);
