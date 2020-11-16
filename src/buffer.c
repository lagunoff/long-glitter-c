#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <cairo.h>

#include "buffer.h"
#include "draw.h"
#include "statusbar.h"
#include "utils.h"
#include "widget.h"

void buffer_view_lines(buffer_t *self);

void buffer_init(buffer_t *self, widget_context_init_t *ctx, char *path) {
  struct stat st;
  self->fd = open(path, O_RDWR);
  self->path = path;
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  input_init(&self->input, ctx, new_bytes(mmaped, st.st_size));
  self->input.ctx.font = &ctx->palette->monospace_font;
  draw_init_context(&self->ctx, ctx);
  self->context_menu.ctx = self->ctx;
  self->context_menu.ctx.window = 0;
  self->statusbar.ctx = self->ctx;
  statusbar_init(&self->statusbar, self);
  self->show_lines = true;
  draw_set_font(&self->input.ctx, &ctx->palette->monospace_font);
  buffer_msg_t layout = {.tag = MSG_LAYOUT};
  buffer_dispatch(self, &layout, &noop_yield);
}

void buffer_free(buffer_t *self) {
  input_free(&self->input);
  bs_free(self->input.contents, lambda(void _(bs_bytes_t *b){
    munmap(b->bytes, b->len);
  }));
  close(self->fd);
}

void buffer_view(buffer_t *self) {
  buffer_view_lines(self);
  input_dispatch(&self->input, (input_msg_t *)&msg_view, &noop_yield);
  statusbar_dispatch(&self->statusbar, (statusbar_msg_t *)&msg_view, &noop_yield);
}

void buffer_view_lines(buffer_t *self) {
  __auto_type ctx = &self->ctx;
  __auto_type line = self->input.scroll.line;
  char temp[64];
  cairo_text_extents_t text_size;
  int y = self->lines.y;
  draw_set_color(ctx, ctx->palette->secondary_text);
  for(;;line++) {
    sprintf(temp, "%d", line + 1);
    draw_measure_text(ctx, temp, strlen(temp), &text_size);
    draw_text(ctx, self->lines.w - 12 - text_size.x_advance, y + ctx->font->extents.ascent, temp, strlen(temp));
    y += ctx->font->extents.height;
    if (y + ctx->font->extents.height >= self->lines.h) break;
  }
}

void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  auto void yield_context_menu(void *msg);
  auto void yield_input(void *msg);
  __auto_type ctx = &self->ctx;

  switch (msg->tag) {
  case Expose: {
    buffer_view(self);
    return;
  }
  case MSG_FREE: {
    buffer_free(self);
    return;
  }
  case MSG_LAYOUT: {
    statusbar_msg_t measure = {.tag = MSG_MEASURE};
    statusbar_dispatch(&self->statusbar, &measure, &noop_yield);

    self->lines.x = ctx->clip.x;
    self->lines.y = ctx->clip.y;
    self->lines.w = self->show_lines ? 64 : 0;
    self->lines.h = ctx->clip.h - measure.measure.y;

    self->input.ctx.clip.x = self->lines.x + self->lines.w;
    self->input.ctx.clip.y = ctx->clip.y;
    self->input.ctx.clip.w = ctx->clip.w - self->lines.w;
    self->input.ctx.clip.h = ctx->clip.h - measure.measure.y;

    self->statusbar.ctx.clip.x = ctx->clip.x;
    self->statusbar.ctx.clip.y = self->input.ctx.clip.y + self->input.ctx.clip.h;
    self->statusbar.ctx.clip.w = ctx->clip.w;
    self->statusbar.ctx.clip.h = measure.measure.y;

    statusbar_dispatch(&self->statusbar, (void *)msg, &noop_yield);
    input_dispatch(&self->input, (void *)msg, &yield_input);
    return;
  }
  case MSG_MEASURE: {
    msg->widget.measure.x = INT_MAX;
    msg->widget.measure.y = INT_MAX;
    return;
  }
  case BUFFER_INPUT: {
    return input_dispatch(&self->input, &msg->input, &yield_input);
  }
  case BUFFER_CONTEXT_MENU: {
    if (msg->context_menu.tag == MENULIST_ITEM_CLICKED) {
      buffer_msg_t next_msg = {.tag = msg->context_menu.item_clicked->action};
      yield(&next_msg);
      widget_close_window(self->context_menu.ctx.window);
      self->context_menu.ctx.window = 0;
      return;
    }
    if (msg->context_menu.tag == MENULIST_DESTROY) {
      widget_close_window(self->context_menu.ctx.window);
      self->context_menu.ctx.window = 0;
      return;
    }
    menulist_dispatch(&self->context_menu, &msg->context_menu, &yield_context_menu);
    if (msg->context_menu.tag == Expose) {
      // SDL_RenderPresent(self->context_menu.ctx.renderer);
    }
    return;
  }}

  void yield_context_menu(void *msg) {
    buffer_msg_t buffer_msg = {.tag = BUFFER_CONTEXT_MENU, .context_menu = *(menulist_msg_t *)msg};
    yield(&buffer_msg);
  }
  void yield_input(void *msg) {
    buffer_msg_t buffer_msg = {.tag = BUFFER_INPUT, .input = *(input_msg_t *)msg};
    yield(&buffer_msg);
  }
}

int buffer_unittest() {
}
