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
#include "c-mode.h"

void buffer_view_lines(buffer_t *self);
syntax_highlighter_t *buffer_choose_syntax_highlighter(char *path);

void buffer_init(buffer_t *self, widget_context_init_t *ctx, char *path) {
  struct stat st;
  self->fd = open(path, O_RDWR);
  self->path = path;
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  self->ctx.font = &ctx->palette->monospace_font;
  draw_init_context(&self->ctx, ctx);
  input_init(&self->input, ctx, new_bytes(mmaped, st.st_size), buffer_choose_syntax_highlighter(path));
  self->context_menu.ctx = self->ctx;
  self->context_menu.ctx.window = 0;
  self->statusbar.ctx = self->ctx;
  statusbar_init(&self->statusbar, self);
  self->show_lines = true;
  draw_set_font(&self->input.ctx, &self->ctx.palette->monospace_font);
}

syntax_highlighter_t *buffer_choose_syntax_highlighter(char *path) {
  __auto_type ext = extension(path); if (!ext) return &noop_highlighter;
  if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) return &c_mode_highlighter;
  return &noop_highlighter;
}

void buffer_free(buffer_t *self) {
  input_free(&self->input);
  bs_free(self->input.contents, lambda(void _(bs_bytes_t *b){
    munmap(b->bytes, b->len);
  }));
  close(self->fd);
}

void buffer_view_lines(buffer_t *self) {
  __auto_type ctx = &self->ctx;
  __auto_type line = self->input.scroll.line;
  __auto_type color = ctx->palette->secondary_text;
  color.alpha = .18;
  char temp[64];
  cairo_text_extents_t text_size;
  int y = self->lines.y;
  draw_set_color(&self->ctx, self->ctx.background);
  draw_rect(&self->ctx, self->lines);

  draw_set_font(&self->ctx, &self->ctx.palette->monospace_font);
  draw_set_color(ctx, color);
  for(int i = 0; i < self->input.lines_len; line++, i++) {
    // File content ended, dont draw line numbers
    if (self->input.lines[i] == -1) break;
    sprintf(temp, "%d", line + 1);
    draw_measure_text(ctx, temp, &text_size);
    draw_text(ctx, self->lines.x + self->lines.w - 12 - text_size.x_advance, y + ctx->font->extents.ascent, temp);
    y += ctx->font->extents.height;
    if (y + ctx->font->extents.height >= self->lines.y + self->lines.h) break;
  }
}

void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  auto void yield_context_menu(void *msg);
  auto void yield_input(void *msg);
  auto void yield_statusbar(void *msg);
  __auto_type ctx = &self->ctx;

  switch (msg->tag) {
  case Expose: {
    if (self->show_lines) buffer_view_lines(self);
    input_dispatch(&self->input, (input_msg_t *)&msg_view, &noop_yield);
    statusbar_dispatch(&self->statusbar, (statusbar_msg_t *)&msg_view, &noop_yield);
    return;
  }
  case MotionNotify: {
    return;
  }
  case KeyPress: {
    // TODO: event has to be redirected only to focused subwidget
    __auto_type xkey = &msg->widget.x_event.xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    if (keysym == XK_F10) {
      self->show_lines = !self->show_lines;
      buffer_msg_t next_msg = {.tag=MSG_LAYOUT};
      yield(&next_msg);
      return yield(&msg_view);
    }
    return yield_input(msg);
  }
  case SelectionRequest: {
    return yield_input(msg);
  }
  case SelectionNotify: {
    return yield_input(msg);
  }
  case MSG_FREE: {
    return buffer_free(self);
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

    yield_statusbar(msg);
    yield_input(msg);
    return;
  }
  case MSG_MEASURE: {
    msg->widget.measure.x = INT_MAX;
    msg->widget.measure.y = INT_MAX;
    return;
  }
  case BUFFER_INPUT: {
    __auto_type prev_offset = bs_offset(&self->input.cursor.pos);
    input_dispatch(&self->input, &msg->input, &yield_input);
    __auto_type next_offset = bs_offset(&self->input.cursor.pos);
    if (prev_offset != next_offset) {
      statusbar_dispatch(&self->statusbar, &msg_view, &noop_yield);
    }
    return;
  }
  case BUFFER_STATUSBAR: {
    return statusbar_dispatch(&self->statusbar, &msg->statusbar, &yield_statusbar);
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
  void yield_statusbar(void *msg) {
    buffer_msg_t buffer_msg = {.tag = BUFFER_STATUSBAR, .input = *(statusbar_msg_t *)msg};
    yield(&buffer_msg);
  }
}

int buffer_unittest() {
}
