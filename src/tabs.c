#include "tabs.h"

void tabs_init(tabs_t *self, widget_context_init_t *ctx, char *path) {
  draw_init_context(&self->ctx, ctx);
  self->last_key = 0;
  self->active = malloc(sizeof(buffer_list_node_t));
  self->active->key = ++self->last_key;
  self->active->next = NULL;
  self->active->prev = NULL;
  buffer_init(&self->active->buffer, ctx, path);
  self->tabs.first = self->active;
  self->tabs.last = self->active;
}

void tabs_free(tabs_t *self) {
  for(__auto_type iter = self->tabs.first; iter; iter = iter->next) {
    buffer_free(&iter->buffer);
  }
}

void tabs_view(tabs_t *self) {
  __auto_type ctx = &self->ctx;
  // Draw contents
  if (self->active) {
    buffer_dispatch(&self->active->buffer, (buffer_msg_t *)&msg_view, &noop_yield);
  }
  // Draw tabs
  draw_set_color_rgba(ctx, 0.97, 0.97, 0.97, 1);
  draw_rect(ctx, self->tabs_clip);
  draw_set_color(ctx, ctx->palette->border);
  cairo_set_antialias(ctx->cairo, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width(ctx->cairo, 1.0);
  draw_line(ctx, self->tabs_clip.x, self->tabs_clip.y + self->tabs_clip.h - 1, self->tabs_clip.x + self->tabs_clip.w, self->tabs_clip.y + self->tabs_clip.h - 1);
  int x = self->tabs_clip.x;
  int y_text = self->tabs_clip.y + (self->tabs_clip.h - ctx->font->extents.ascent) * 0.5 + ctx->font->extents.ascent;
  for(__auto_type iter = self->tabs.first; iter; iter = iter->next) {
    char *fname = basename(iter->buffer.path);
    int x_padding = 16;
    cairo_text_extents_t extents;
    draw_measure_text(ctx, fname, &extents);
    draw_set_color_rgba(ctx, 1, 1, 1, 1);
    draw_rectangle(ctx, x, self->tabs_clip.y, x + extents.x_advance + x_padding * 2, self->tabs_clip.h);
    draw_set_color(ctx, ctx->palette->border);
    draw_line(ctx, x + extents.x_advance + x_padding * 2, self->tabs_clip.y, x + extents.x_advance + x_padding * 2, self->tabs_clip.y + self->tabs_clip.h);
    draw_set_color(ctx, ctx->palette->secondary_text);
    draw_text(ctx, x + x_padding, y_text, fname);
  }
}

void tabs_dispatch(tabs_t *self, tabs_msg_t *msg, yield_t yield) {
  auto void yield_active(buffer_msg_t *buffer_msg);
  auto void yield_key(buffer_msg_t *buffer_msg);
  __auto_type ctx = &self->ctx;
  int current_key = -1;

  switch(msg->tag) {
  case Expose: {
    return tabs_view(self);
  }
  case MSG_FREE: {
    return tabs_free(self);
  }
  case MotionNotify: {
    return yield_active((buffer_msg_t *)msg);
  }
  case KeyPress: {
    return yield_active((buffer_msg_t *)msg);
  }
  case SelectionRequest: {
    return yield_active((buffer_msg_t *)msg);
  }
  case SelectionNotify: {
    return yield_active((buffer_msg_t *)msg);
  }
  case MSG_LAYOUT: {
    __auto_type tabs_height = 36;
    __auto_type iter = self->tabs.first;
    rect_t tab_content_clip = {ctx->clip.x, ctx->clip.y + tabs_height, ctx->clip.w, ctx->clip.h - tabs_height};
    for(; iter; iter = iter->next) {
      iter->buffer.ctx.clip = tab_content_clip;
      current_key = iter->key;
      yield_key((buffer_msg_t *)msg);
    }
    self->tabs_clip.x = ctx->clip.x; self->tabs_clip.y = ctx->clip.y;
    self->tabs_clip.w = ctx->clip.w; self->tabs_clip.h = tabs_height;
    return;
  }
  case TABS_ACTIVE: {
    return yield_active(&msg->active);
  }
  case TABS_KEY: {
    __auto_type iter = self->tabs.first;
    for(; iter; iter = iter->next) {
      if (iter->key == msg->key.key) break;
    }
    if (!iter) return;
    current_key = iter->key;
    return buffer_dispatch(&iter->buffer, &msg->key.msg, (yield_t)&yield_key);
  }}

  void yield_active(buffer_msg_t *buffer_msg) {
    if (!self->active) return;
    tabs_msg_t next_msg = {.tag=TABS_KEY, .key={.key=self->active->key, .msg=*buffer_msg}};
    return yield(&next_msg);
  }
  void yield_key(buffer_msg_t *buffer_msg) {
    tabs_msg_t next_msg = {.tag=TABS_KEY, .key={.key=current_key, .msg=*buffer_msg}};
    yield(&next_msg);
  }
}
