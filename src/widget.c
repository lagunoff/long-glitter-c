#include <X11/Xlib.h>
#include "widget.h"

void noop_yield(void *msg) {}

void widget_close_window(Window window) {}

static __attribute__((constructor)) void __init__() {
  msg_view.tag = Expose;
  msg_noop.tag = MSG_NOOP;
  msg_free.tag = MSG_FREE;
  msg_layout.tag = MSG_LAYOUT;
}
