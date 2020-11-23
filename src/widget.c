#include <X11/Xlib.h>
#include "widget.h"

void noop_yield(void *msg) {}

void widget_close_window(Window window) {}

static __attribute__((constructor)) void __init__() {
  msg_view.tag = Expose;
  msg_noop.tag = Widget_Noop;
  msg_free.tag = Widget_Free;
  msg_layout.tag = Widget_Layout;
}
