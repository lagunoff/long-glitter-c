#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "widget.h"

typedef struct {
  char *title;
  bool  disabled;
  int   icon; // Glyph codepoint from fontawesome font, 0 if the item lacks icon
  int   action;
} menulist_item_t;

typedef struct {
  draw_context_t   ctx;
  menulist_item_t *items; //! aligned by value of 'alignment' field
  int              len;
  int              alignement; //! sizeof(specific_item_t)
  int              hover;
} menulist_t;

typedef union {
  widget_msg_t widget;

  struct {
    enum {
      MENULIST_PARENT_WINDOW = MSG_USER,
      MENULIST_ITEM_CLICKED,
      MENULIST_DESTROY,
    } tag;

    union {
      SDL_Event       *parent_sdl_event;
      menulist_item_t *item_clicked;
    };
  };
} menulist_msg_t;

menulist_msg_t menulist_destroy;
menulist_msg_t menulist_parent_window(SDL_Event *event);
menulist_msg_t menulist_item_clicked(menulist_item_t *item);

void menulist_init(menulist_t *self, menulist_item_t *items, int len, int alignement);
void menulist_dispatch(menulist_t *self, menulist_msg_t *msg, yield_t yield);
