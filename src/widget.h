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

typedef struct {
  widget_context_t *instance;
  widget_t dispatch;
  yield_t  yield;
} child_widget_t;

#define redirect_x_events(init_widgets) {                               \
  child_widget_t widgets[] = init_widgets;                              \
  switch(msg->tag) {                                                    \
  case MotionNotify: {                                                  \
    __auto_type motion = &msg->widget.x_event.xmotion;                  \
    for (int i = 0; i < sizeof(widgets) / sizeof(widgets[0]); i++) {    \
      if (rect_is_inside(widgets[i].instance->clip, motion->x, motion->y)) { \
        return widgets[i].yield((void *)msg);                           \
      }                                                                 \
    }                                                                   \
    return;                                                             \
  }                                                                     \
  case ButtonPress: {                                                   \
    __auto_type button = &msg->widget.x_event.xbutton;                  \
    for (int i = 0; i < sizeof(widgets) / sizeof(widgets[0]); i++) {    \
      if (rect_is_inside(widgets[i].instance->clip, button->x, button->y)) { \
        return widgets[i].yield((void *)msg);                           \
      }                                                                 \
    }                                                                   \
    return;                                                             \
  }}}
