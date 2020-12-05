#pragma once
#include <stdbool.h>
#include <cairo.h>
#include <X11/Xlib.h>

#include "utils.h"

union widget_msg_t;
struct palette_t;
struct font_t;
struct base_widget_t;

typedef struct {
  double red, green, blue, alpha;
} color_t;

typedef void (*yield_t)(void *msg);
typedef void (*dispatch_t)(void *self, union widget_msg_t *msg, yield_t yield);
typedef bool (*lookup_cb_t)(dispatch_t dispatch, struct base_widget_t *widget);

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

typedef struct base_widget_t {
  rect_t     clip;
  dispatch_t dispatch;
} base_widget_t;

typedef struct {
  dispatch_t     dispatch;
  base_widget_t *widget;
} some_widget_t;

typedef union widget_msg_t {
  XEvent x_event[0];
  struct {
    enum {
      Widget_Noop = LASTEvent,
      Widget_View,
      Widget_Free,
      Widget_Measure,
      Widget_Layout,
      Widget_MouseEnter,
      Widget_MouseLeave,
      Widget_FocusIn,
      Widget_FocusOut,
      Widget_Children,
      Widget_Last,
    } tag;
    union {
      point_t measure;
      struct {
        bool focused;
        bool hovered;
      } view;
      struct {
        some_widget_t some;
        void         *msg;
      } children;
    };
  };
} widget_msg_t;

static widget_msg_t msg_noop = {.tag = Widget_Noop};
static widget_msg_t msg_free = {.tag = Widget_Free};
static widget_msg_t msg_view = {.tag = Expose};
static widget_msg_t msg_layout = {.tag = Widget_Layout};
static widget_msg_t mouse_enter = {.tag = Widget_MouseEnter};
static widget_msg_t mouse_leave = {.tag = Widget_MouseLeave};
static widget_msg_t focus_in = {.tag = Widget_FocusIn};
static widget_msg_t focus_out = {.tag = Widget_FocusOut};
static void noop_yield(void *msg) {}

void widget_close_window(Window window);

typedef struct {
  base_widget_t *focus;
  base_widget_t *hover;
} container_t;

typedef struct {
  enum {
    Lookup_Empty,
    Lookup_Coords,
    Lookup_ButtonPress
  } tag;
  union {
    point_t coords;
    point_t button_press;
  };
} lookup_filter_t;

static void noop_dispatch(void *self, union widget_msg_t *msg, yield_t yield) {}
static lookup_filter_t noop_filter = {.tag = Lookup_Empty};
static some_widget_t noop_widget = {.dispatch = &noop_dispatch, .widget = NULL};

#define dispatch_some(__dispatch, __widget, __msg)     ({               \
  (__widget).dispatch((__widget).widget, (widget_msg_t *)__msg, lambda(void _(void *m) {\
    widget_msg_t next_msg;                                              \
    next_msg.tag = Widget_Children;                                     \
    next_msg.children.some = (__widget);                                        \
    next_msg.children.msg = m;                                          \
    __dispatch(self, (void *)&next_msg, yield);                         \
  }));                                                                  \
  })

#define yield_children(__dispatch, __widget, __msg)                     \
  yield(&(widget_msg_t){.tag=Widget_Children, .children={{(dispatch_t)&__dispatch, (base_widget_t *)(__widget)}, __msg}})

#define redirect_x_events(__dispatch) {                                 \
  switch(msg->tag) {                                                    \
  case MotionNotify: {                                                  \
    __auto_type xmotion = &msg->widget.x_event->xmotion;                \
    __auto_type filter = (lookup_filter_t){.tag= Lookup_Coords, .coords={xmotion->x, xmotion->y}}; \
    __auto_type next_hover = lookup_children(filter);                   \
    if (next_hover.widget != self->hover.widget) {                      \
      dispatch_some(__dispatch, self->hover, &mouse_leave);             \
      dispatch_some(__dispatch, next_hover, &mouse_enter);              \
      self->hover.dispatch = next_hover.dispatch;                       \
      self->hover.widget = next_hover.widget;                           \
    } else {                                                            \
      dispatch_some(__dispatch, self->hover, msg);                      \
    }                                                                   \
    break;                                                              \
  }                                                                     \
  case ButtonPress: {break;                                             \
    __auto_type xbutton = &msg->widget.x_event->xbutton;                \
    if (xbutton->button != Button1) break;                              \
    __auto_type filter = (lookup_filter_t){.tag=Lookup_ButtonPress, .button_press={xbutton->x, xbutton->y}}; \
    __auto_type next_focus = lookup_children(filter);                   \
    if (next_focus.widget != self->focus.widget) {                      \
      dispatch_some(__dispatch, self->focus, &focus_out);               \
      dispatch_some(__dispatch, next_focus, &focus_in);                 \
      self->focus.dispatch = next_focus.dispatch;                       \
      self->focus.widget = next_focus.widget;                           \
    }                                                                   \
    break;                                                              \
  }                                                                   \
  case KeyPress: {                                                   \
    dispatch_some(__dispatch, self->focus, msg);                        \
    break;                                                              \
  }                                                                     \
  case SelectionRequest: {                                              \
    dispatch_some(__dispatch, self->focus, msg);                        \
    break;                                                              \
  }                                                                     \
  case SelectionNotify: {                                             \
    dispatch_some(__dispatch, self->focus, msg);                        \
    break;                                                              \
  }                                                                     \
  case Widget_Children: {                                             \
    dispatch_some(__dispatch, msg->widget.children.some, msg->widget.children.msg); \
    break;                                                              \
  }}}
