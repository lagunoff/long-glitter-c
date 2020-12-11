#pragma once
#include <stdbool.h>
#include <cairo.h>
#include <X11/Xlib.h>

#include "utils.h"

union widget_msg_t;
struct palette_t;
struct font_t;
union new_widget_t;
struct widget_rect_t;
struct widget_basic_t;
struct widget_container_t;

typedef enum {
  Widget_Rectangle,
  Widget_Basic,
  Widget_Container,
} widget_tag_t;

typedef struct {
  double red, green, blue, alpha;
} color_t;

typedef void (*yield_t)(void *msg);
typedef void (*dispatch_t)(void *self, void *msg, yield_t yield);

typedef struct {
  dispatch_t            dispatch;
  struct base_widget_t *widget;
} some_widget_t;

typedef struct {
  Window   window;
  Display *display;
  cairo_t *cairo;
  struct palette_t *palette;
  XIC      xic;
  struct font_t *font;
} widget_context_t;

typedef struct widget_rect_t {
  widget_tag_t tag;
  rect_t       clip;
} widget_rect_t;

typedef struct widget_basic_t {
  widget_tag_t  tag;
  rect_t        clip;
  widget_context_t *ctx;
  dispatch_t    dispatch;
} widget_basic_t;

typedef struct widget_container_t {
  widget_tag_t  tag;
  rect_t        clip;
  widget_context_t *ctx;
  dispatch_t    dispatch;
  union new_widget_t *hover;
  union new_widget_t *focus;
} widget_container_t;

typedef union new_widget_t {
  widget_tag_t       tag;
  widget_rect_t      rect;
  widget_basic_t     basic;
  widget_container_t container;
} new_widget_t;

typedef struct {
  enum {
    Lookup_Empty,
    Lookup_Coords,
  } tag;
  union {
    point_t coords;
  };
} lookup_filter_t;

typedef new_widget_t *(*lookup_t)(void *self, lookup_filter_t filter);

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
      Widget_NewChildren,
      Widget_Lookup,
      Widget_NewWindow,
      Widget_ConsNewWindow,
      Widget_Last,
    } tag;
    union {
      point_t measure;
      struct {
        bool focused;
        bool hovered;
      } view;
      struct {
        new_widget_t *widget;
        void         *msg;
      } new_children;
      struct {
        lookup_filter_t request;
        new_widget_t   *response;
      } lookup;
      Window new_window;
      struct {
        union widget_msg_t *cdr;
        union new_widget_t *car;
      } cons_new_window;
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

static void noop_dispatch(void *self, void *msg, yield_t yield) {}
static lookup_filter_t noop_filter = {.tag = Lookup_Empty};

static void dispatch_to(yield_t yield, void *w, void *m) {
  if (w == NULL) return;
  __auto_type msg = (widget_msg_t *)m;
  __auto_type widget = (new_widget_t *)w;

  bool does_bubble(widget_msg_t *msg) {
    return msg->tag >= Widget_Last;
  }
  void yield_next(void *next_msg) {
    if (does_bubble(next_msg)) {
      yield(&(widget_msg_t){.tag=Widget_NewChildren, .new_children={widget, next_msg}});
    } else {
      widget->basic.dispatch(widget, next_msg, &yield_next);
    }
  }

  if (widget->tag == Widget_Container || widget->tag == Widget_Basic) {
    widget->basic.dispatch(widget, msg, &yield_next);
  }
}

#define yield_children(__dispatch, __widget, __msg)                     \
  yield(&(widget_msg_t){.tag=Widget_Children, .children={{(dispatch_t)&__dispatch, (base_widget_t *)(__widget)}, (__msg)}})

static void sync_container(void *s, void *m, yield_t yield, dispatch_t next) {
  __auto_type msg = (widget_msg_t *)m;
  __auto_type self = (new_widget_t *)s;
  if (self->tag != Widget_Container) return;

  switch(msg->tag) {
  case MotionNotify: {
    __auto_type xmotion = &msg->x_event->xmotion;
    __auto_type lookup_msg = (widget_msg_t){.tag=Widget_Lookup, .lookup={{Lookup_Coords,{xmotion->x, xmotion->y}}, NULL}};
    __auto_type next_hover = lookup_msg.lookup.response;
    if (next_hover != self->container.hover) {
      dispatch_to(yield, self->container.hover,  &mouse_leave);
      dispatch_to(yield, next_hover, &mouse_enter);
      self->container.hover = next_hover;
    } else {
      dispatch_to(yield, self->container.hover, msg);
    }
    break;
  }
  case ButtonPress: {
    __auto_type xbutton = &msg->x_event->xbutton;
    __auto_type lookup_msg = (widget_msg_t){.tag=Widget_Lookup, .lookup={{Lookup_Coords,{xbutton->x, xbutton->y}}, NULL}};
    __auto_type next_focus = lookup_msg.lookup.response;
    dispatch_to(yield, next_focus, msg);
    if (0 && next_focus != self->container.focus) {
      dispatch_to(yield, self->container.focus, &focus_out);
      dispatch_to(yield, next_focus, &focus_in);
      self->container.focus = next_focus;
    }
    break;
  }
  case KeyPress: {
    dispatch_to(yield, self->container.focus, msg);
    break;
  }
  case SelectionRequest: {
    dispatch_to(yield, self->container.focus, msg);
    break;
  }
  case SelectionNotify: {
    dispatch_to(yield, self->container.focus, msg);
    break;
  }
  case Widget_NewChildren: {
    dispatch_to(yield, msg->new_children.widget, msg->new_children.msg);
    break;
  }}
}

// There is better way to do this, that checks that w is one of the
// widgets types, not just any pointer
#define coerce_widget(w) ((new_widget_t *)w)
