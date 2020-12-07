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

void address_line_init(address_line_t *self, widget_context_t *ctx, struct buffer_t *buffer) {
  self->widget.ctx = ctx;
  self->widget.dispatch = (dispatch_t)&address_line_dispatch;
  self->font = &ctx->palette->small_font;
  self->buffer = buffer;
  input_init(&self->input, ctx, new_bytes(buffer->path, strlen(buffer->path)), &noop_highlighter);
  self->input.font = self->font;
  self->input.highlight_line = false;
  self->focus = (some_widget_t){(dispatch_t)&input_dispatch, (base_widget_t *)&self->input};
  self->hover = noop_widget;
}

static int y_padding = 1;
static int x_padding = 8;

void address_line_dispatch(address_line_t *self, address_line_msg_t *msg, yield_t yield) {
  auto some_widget_t lookup_children(lookup_filter_t filter);

  switch(msg->tag) {
  case Expose: {
    input_dispatch(&self->input, (input_msg_t *)msg, &noop_yield);
    return;
  }
  case Widget_FocusIn: {
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
  }}
  redirect_x_events(address_line_dispatch);

  some_widget_t lookup_children(lookup_filter_t filter) {
    return (some_widget_t){(dispatch_t)&input_dispatch, (base_widget_t *)&self->input};
  }
}
