#pragma once
#include <stdbool.h>
#include <X11/Xlib.h>

#include "draw.h"
#include "utils.h"

typedef union {
  XEvent x_event;
  struct {
    enum {
      Widget_Noop = LASTEvent,
      Widget_Free,
      Widget_Measure,
      Widget_Layout,
      Widget_Last,
    } tag;
    union {
      point_t measure;
    };
  };
} widget_msg_t;

typedef void (*yield_t)(void *msg);
typedef void (*widget_t)(void *self, widget_msg_t *msg, yield_t yield);

widget_msg_t msg_noop;
widget_msg_t msg_free;
widget_msg_t msg_view;
widget_msg_t msg_layout;

void widget_close_window(Window window);
void noop_yield(void *msg);
