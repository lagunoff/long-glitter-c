#include <X11/Xlib.h>
#include <stdlib.h>
#include "widget.h"
#include "graphics.h"

void dispatch_to(yield_t yield, void *w, void *m) {
  if (w == NULL) return;
  __auto_type msg = (widget_msg_t *)m;
  __auto_type widget = (new_widget_t *)w;

  bool does_bubble(widget_msg_t *msg) {
    return msg->tag >= Widget_AskContext;
  }
  void yield_next(void *next_msg) {
    __auto_type next_widget_msg = (widget_msg_t *)next_msg;
    if (next_widget_msg->tag == Expose) {
      gx_set_color(widget->basic.ctx, gx_rgba(1,1,1,0));
      gx_box(widget->basic.ctx, widget->basic.clip.x, widget->basic.clip.y, widget->basic.clip.w, widget->basic.clip.h);
    }
    if (does_bubble(next_msg)) {
      yield(&(widget_msg_t){.tag=Widget_NewChildren, .new_children={widget, next_msg}});
    } else {
      widget->basic.dispatch(widget, next_msg, &yield_next);
    }
  }
  if (msg->tag == Expose) {
    gx_set_color(widget->basic.ctx, gx_rgba(1,1,1,0));
    gx_box(widget->basic.ctx, widget->basic.clip.x, widget->basic.clip.y, widget->basic.clip.w, widget->basic.clip.h);
  }

  if (widget->tag == Widget_Container || widget->tag == Widget_Basic) {
    widget->basic.dispatch(widget, msg, &yield_next);
  }
}

void view_to(void *w, yield_t yield) {
  dispatch_to(yield, w, &msg_view);
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
      if (child_msg->new_window.msg == (void**)&child_msg->new_window.msg_head) {
        child_msg->new_window.msg = (void**)&new_head->new_children.msg;
      }
      child_msg->new_window.msg_head = new_head;
      return yield(child_msg);
    }
    if (child_msg->tag == Widget_CloseWindow) {
      return yield(child_msg);
    }
    return dispatch_to(yield, msg->new_children.widget, msg->new_children.msg);
  }}
  return next(s, m, yield);
}

void widget_open_window(new_widget_t *self, Window window, int width, int height, yield_t yield) {
  self->basic.clip = (rect_t){0,0,width,height};
  yield(&(widget_msg_t){.tag=Widget_NewChildren, .new_children={self,&msg_layout}});
  __auto_type new_window_msg = (widget_msg_t){.tag=Widget_NewWindow, .new_window={
    window, self, NULL, NULL, width, height,
  }};
  new_window_msg.new_window.msg = (void**)&new_window_msg.new_window.msg_head;
  return yield(&(widget_msg_t){.tag=Widget_NewChildren, .new_children={self,&new_window_msg}});
}

void widget_close_window(Window window, yield_t yield) {
  return yield(&(widget_msg_t){.tag=Widget_CloseWindow, .close_window=window});
}
