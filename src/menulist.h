#pragma once
#include <stdbool.h>

#include "widget.h"

typedef struct {
  char *title;
  bool  disabled;
  int   icon; // Glyph codepoint from fontawesome font, 0 if the item lacks icon
  int   action;
} menulist_item_t;

typedef struct {
  widget_context_t   ctx;
  menulist_item_t *items; //! aligned by value of 'alignment' field
  int              len;
  int              alignement; //! sizeof(specific_item_t)
  int              hover;
} menulist_t;

typedef union {
  widget_msg_t widget;
  XEvent       x_event;

  struct {
    enum {
      Menulist_DispatchParent = Widget_Last,
      Menulist_ItemClicked,
      Menulist_Destroy,
    } tag;

    union {
      XEvent          *parent_x_event;
      menulist_item_t *item_clicked;
    };
  };
} menulist_msg_t;

void menulist_init(menulist_t *self, menulist_item_t *items, int len, int alignement);
void menulist_dispatch(menulist_t *self, menulist_msg_t *msg, yield_t yield);
