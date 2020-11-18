#pragma once
#include <X11/Xlib.h>

#include "cursor.h"
#include "draw.h"
#include "widget.h"

struct buffer_t;

typedef struct {
  widget_context_t ctx;
  struct buffer_t *buffer;
} statusbar_t;

typedef widget_msg_t statusbar_msg_t;

void statusbar_init(statusbar_t *self, struct buffer_t *buffer);
void statusbar_view(statusbar_t *self);
void statusbar_dispatch(statusbar_t *self, statusbar_msg_t *msg, yield_t yield);
