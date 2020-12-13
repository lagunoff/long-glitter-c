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
  Window   window;
  Display *display;
  cairo_t *cairo;
  struct palette_t *palette;
  XIC      xic;
  struct font_t *font;
} widget_context_t;

typedef struct widget_rect_t {
  widget_tag_t tag;
  rect_t       clip; // FIXME: Take into account scroll position
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
      Widget_TabCycle,
      Widget_AskContext,
      Widget_Last,
    } tag;
    union {
      point_t measure;
      struct {
        new_widget_t *widget;
        void         *msg;
      } new_children;
      struct {
        lookup_filter_t request;
        new_widget_t   *response;
      } lookup;
      struct {
        Window        window;
        union widget_msg_t *msg_head;
        void        **msg;
      } new_window;
      struct {
        bool focused;
        bool hovered;
      } ask_context;
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
static lookup_filter_t noop_filter = {.tag = Lookup_Empty};

static void noop_dispatch(void *self, void *msg, yield_t yield) {}
static void noop_yield(void *msg) {}

void widget_close_window(Window window);
void dispatch_to(yield_t yield, void *w, void *m);
void sync_container(void *s, void *m, yield_t yield, dispatch_t next);

// There is better way to do this, that checks that w is one of the
// widgets types, not just any pointer
#define coerce_widget(w) ((new_widget_t *)w)

typedef struct widget_window_node_t {
  struct widget_window_node_t *next;
  struct widget_window_node_t *prev;
  Window        window;
  widget_msg_t *msg_head;
  void        **msg;
} widget_window_node_t;

typedef struct {
  widget_window_node_t *first;
  widget_window_node_t *last;
} widget_window_head_t;
