#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "address-line.h"
#include "buffer.h"
#include "graphics.h"
#include "utils.h"
#include "buff-string.h"

static menulist_item_t autocomplete_items[] = {
  {.title="sdfsd",.disabled=false},
  {.title="sdfsdsdfsd",.disabled=false},
};

void address_line_init(address_line_t *self, widget_context_t *ctx, struct buffer_t *buffer) {
  self->widget = (widget_container_t){
    Widget_Container, {0,0,0,0}, ctx,
    (dispatch_t)&address_line_dispatch, NULL, NULL,
  };
  self->font = &ctx->palette->small_font;
  self->buffer = buffer;
  input_init(&self->input, ctx, new_bytes(buffer->path, strlen(buffer->path)), &noop_highlighter);
  self->input.font = self->font;
  self->input.highlight_line = false;
  self->autocomplete_window = -1;
  menulist_init(&self->autocomplete, NULL, autocomplete_items, sizeof(autocomplete_items)/sizeof(autocomplete_items[0]), sizeof(autocomplete_items[0]));
}

static int y_padding = 1;
static int x_padding = 8;

void address_line_dispatch_(address_line_t *self, address_line_msg_t *msg, yield_t yield, dispatch_t next) {
  __auto_type ctx = self->widget.ctx;

  switch(msg->tag) {
  case Expose: {
    input_dispatch(&self->input, (input_msg_t *)msg, &noop_yield);
    return;
  }
  case Widget_FocusIn: {
    if (self->autocomplete_window != -1) {
      XDestroyWindow(ctx->display, self->autocomplete_window);
    }
    self->widget.focus = coerce_widget(&self->autocomplete.widget);
    self->autocomplete_window = XCreateSimpleWindow(ctx->display, ctx->window, self->widget.clip.x, self->widget.clip.y + self->widget.clip.h, self->widget.clip.w, 200, 0, 0, ULONG_MAX);
    return widget_open_window((new_widget_t*)&self->autocomplete, self->autocomplete_window, self->widget.clip.w, 200, yield);
  }
  case Widget_FocusOut: {
    if (self->autocomplete_window != -1) {
      widget_close_window(self->autocomplete_window, yield);
      XDestroyWindow(ctx->display, self->autocomplete_window);
      self->autocomplete_window = -1;
      self->widget.focus = NULL;
    }
    return;
  }
  case Widget_Measure: {
    msg->widget.measure.y = self->font->extents.height + y_padding * 2;
    return;
  }
  case Widget_Layout: {
    self->input.widget.clip = (rect_t){
      self->widget.clip.x + x_padding,
      self->widget.clip.y,
      self->widget.clip.w - x_padding * 2,
      self->widget.clip.h,
    };
  }
  case Widget_Lookup: {
    msg->widget.lookup.response = coerce_widget(&self->input.widget);
    return;
  }
  case Widget_NewChildren: {
    if (msg->widget.new_children.widget->basic.dispatch == (dispatch_t)&menulist_dispatch) {
      __auto_type child_msg = (menulist_msg_t*)msg->widget.new_children.msg;
      if (child_msg->tag == Menulist_Destroy) {
        return yield(&(widget_msg_t){.tag=Widget_FocusOut});
      }
    }
    break;
  }}
  return next(self, msg, yield);
}

void address_line_dispatch(address_line_t *self, address_line_msg_t *msg, yield_t yield) {
  void step_1(address_line_t *self, address_line_msg_t *msg, yield_t yield) {
    return sync_container(self, msg, yield, &noop_dispatch);
  }
  return address_line_dispatch_(self, msg, yield, (dispatch_t)&step_1);
}
