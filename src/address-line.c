#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "autocomplete.h"
#include "address-line.h"
#include "buffer.h"
#include "graphics.h"
#include "utils.h"
#include "buff-string.h"

void address_line_init(address_line_t *self, widget_context_t *ctx, struct buffer_t *buffer) {
  self->widget = (widget_container_t){
    Widget_Container, {0,0,0,0}, ctx,
    (dispatch_t)&address_line_dispatch, NULL, coerce_widget(&self->input),
  };
  self->font = &ctx->palette->small_font;
  self->buffer = buffer;
  input_init(&self->input, ctx, new_bytes(buffer->path, strlen(buffer->path)), &noop_highlighter);
  self->input.font = self->font;
  self->input.highlight_line = false;
  self->autocomplete_window = -1;
  menulist_init(&self->autocomplete, NULL, NULL, 0, sizeof(menulist_item_t));
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
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event->xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    if (keysym == XK_Up) {
      return dispatch_to(yield, &self->autocomplete, &(menulist_msg_t){.tag=Menulist_FocusUp});
    }
    if (keysym == XK_Down) {
      return dispatch_to(yield, &self->autocomplete, &(menulist_msg_t){.tag=Menulist_FocusDown});
    }
    if (keysym == XK_Return) {
      if (!self->autocomplete.widget.focus) return;
      return yield(&(address_line_msg_t){.tag=AddressLine_ItemClicked, .item_clicked=(void *)self->autocomplete.widget.focus});
    }
    return next(self, msg, yield);
  }
  case Widget_FocusIn: {
    if (self->autocomplete_window != -1) {
      XDestroyWindow(ctx->display, self->autocomplete_window);
    }
    // self->widget.focus = coerce_widget(&self->autocomplete.widget);
    self->autocomplete_window = XCreateSimpleWindow(ctx->display, ctx->window, self->widget.clip.x, self->widget.clip.y + self->widget.clip.h, self->widget.clip.w, 400, 0, 0, ULONG_MAX);
    return widget_open_window((new_widget_t*)&self->autocomplete, self->autocomplete_window, self->widget.clip.w, 400, yield);
  }
  case Widget_FocusOut: {
    if (self->autocomplete_window != -1) {
      widget_close_window(self->autocomplete_window, yield);
      XDestroyWindow(ctx->display, self->autocomplete_window);
      self->autocomplete_window = -1;
      // self->widget.focus = NULL;
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
    if (msg->widget.new_children.widget->basic.dispatch == (dispatch_t)&input_dispatch) {
      __auto_type child_msg = (input_msg_t*)msg->widget.new_children.msg;
      next(self, msg, yield);
      if (child_msg->tag == KeyPress) {
        __auto_type contents = bs_copy_stringz(self->input.contents);
        autocomplete_list_free(&self->autocomplete.items, &self->autocomplete.len);
        autocomplete_list_init_filename(contents, &self->autocomplete.items, &self->autocomplete.len);
        dispatch_to(yield, &self->autocomplete, &msg_view);
        free(contents);
      }
      return;
    }
    if (msg->widget.new_children.widget->basic.dispatch == (dispatch_t)&menulist_dispatch) {
      __auto_type child_msg = (menulist_msg_t*)msg->widget.new_children.msg;
      if (child_msg->tag == Menulist_Destroy) {
        return yield(&(widget_msg_t){.tag=Widget_FocusOut});
      }
      if (child_msg->tag == Menulist_ItemClicked) {
        yield(&(address_line_msg_t){.tag=AddressLine_ItemClicked, .item_clicked=child_msg->item_clicked});
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
