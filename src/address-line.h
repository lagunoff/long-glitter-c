#pragma once
#include <X11/Xlib.h>

#include "cursor.h"
#include "graphics.h"
#include "input.h"

struct buffer_t;

typedef struct {
  widget_container_t widget;
  struct buffer_t *buffer;
  font_t          *font;
  input_t          input;
  menulist_t       autocomplete;
  Window           autocomplete_window;
} address_line_t;

typedef union {
  widget_msg_t widget;
  struct {
    enum { AddressLine_Open = Widget_Last, } tag;
  };
} address_line_msg_t;

void address_line_init(address_line_t *self, widget_context_t *ctx, struct buffer_t *buffer);
void address_line_dispatch(address_line_t *self, address_line_msg_t *msg, yield_t yield);
