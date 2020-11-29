#pragma once
#include <stdbool.h>
#include <cairo.h>
#include <X11/Xlib.h>

#include "utils.h"

typedef struct {
  double red, green, blue, alpha;
} color_t;

union widget_msg_t;
struct palette_t;
struct font_t;

typedef void (*yield_t)(void *msg);
typedef void (*dispatch_t)(void *self, union widget_msg_t *msg, yield_t yield);

typedef struct {
  Window  window;
  Display *display;
  cairo_t *cairo;
  struct palette_t *palette;
  XIC      xic;
  struct font_t *font;
} widget_context_t;

typedef struct {
  rect_t            clip;
  dispatch_t        dispatch;
  widget_context_t *ctx;
} widget_t;

typedef union widget_msg_t {
  XEvent x_event[0];
  struct {
    enum {
      Widget_Noop = LASTEvent,
      Widget_Free,
      Widget_Measure,
      Widget_Layout,
      Widget_Embedded,
      Widget_Last,
    } tag;
    union {
      point_t measure;
      struct {
        widget_t           *widget;
        union widget_msg_t *msg;
      } embedded;
    };
  };
} widget_msg_t;

widget_msg_t msg_noop;
widget_msg_t msg_free;
widget_msg_t msg_view;
widget_msg_t msg_layout;

void widget_close_window(Window window);
void noop_yield(void *msg);

typedef struct {
  widget_context_t *keyboard_focus;
  widget_context_t *mouse_focus;
} container_t;

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
