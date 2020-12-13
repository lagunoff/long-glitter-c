#include <X11/Xlib.h>
#include <stdlib.h>
#include "widget.h"

void widget_close_window(Window window) {}

void dispatch_to(yield_t yield, void *w, void *m) {
  if (w == NULL) return;
  __auto_type msg = (widget_msg_t *)m;
  __auto_type widget = (new_widget_t *)w;

  bool does_bubble(widget_msg_t *msg) {
    return msg->tag >= Widget_AskContext;
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

void sync_container(void *s, void *m, yield_t yield, dispatch_t next) {
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
    return;
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
    return;
  }
  case KeyPress: {
    return dispatch_to(yield, self->container.focus, msg);
  }
  case SelectionRequest: {
    return dispatch_to(yield, self->container.focus, msg);
  }
  case SelectionNotify: {
    return dispatch_to(yield, self->container.focus, msg);
  }
  case Widget_NewChildren: {
    __auto_type child_msg = (widget_msg_t *)msg->new_children.msg;
    if (child_msg->tag == Widget_AskContext) {
      child_msg->ask_context.focused = self->container.focus == msg->new_children.widget;
      child_msg->ask_context.hovered = self->container.hover == msg->new_children.widget;
      return;
    }
    if (child_msg->tag == Widget_NewWindow) {
      __auto_type new_head = new(((widget_msg_t){.tag=Widget_NewChildren, .new_children={msg->new_children.widget, child_msg->new_window.msg_head}}));
      child_msg->new_window.msg_head = new_head;
      return yield(child_msg);
    }
    return dispatch_to(yield, msg->new_children.widget, msg->new_children.msg);
  }}
  return next(s, m, yield);
}
