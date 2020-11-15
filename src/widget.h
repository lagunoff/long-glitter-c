#pragma once
#include <stdbool.h>
#include <X11/Xlib.h>

#include "draw.h"
#include "utils.h"

typedef struct {
  enum {
    MSG_NOOP = LASTEvent,
    MSG_VIEW,
    MSG_FREE,
    MSG_MEASURE,
    MSG_LAST,
  } tag;
  union {
    point_t *measure;
  };
} widget_msg_t;

typedef void (*yield_t)(void *msg);
typedef void (*widget_t)(void *self, widget_msg_t *msg, yield_t yield);

widget_msg_t msg_noop;
widget_msg_t msg_free;
widget_msg_t msg_view;

inline widget_msg_t msg_measure(point_t *result) {
  widget_msg_t msg = {.tag = MSG_MEASURE, .measure = result};
  return msg;
}

void widget_close_window(Window window);
void noop_yield(void *msg);
