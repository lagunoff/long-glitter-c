#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "buffer.h"
#include "draw.h"
#include "statusbar.h"
#include "utils.h"
#include "widget.h"

void buffer_view_lines(buffer_t *self);

void buffer_init(buffer_t *self, draw_context_init_t *ctx, char *path) {
  struct stat st;
  self->fd = open(path, O_RDWR);
  self->path = path;
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  input_init(&self->input, ctx, new_bytes(mmaped, st.st_size));
  self->input.ctx.font = ctx->ro->palette->monospace_font;
  draw_init_context(&self->ctx, ctx);
  self->context_menu.ctx = self->ctx;
  self->context_menu.ctx.window = 0;
  self->statusbar.ctx = self->ctx;
  statusbar_init(&self->statusbar, self);
  self->show_lines = true;
}

void buffer_free(buffer_t *self) {
  input_free(&self->input);
  bs_free(self->input.contents, lambda(void _(bs_bytes_t *b){
    munmap(b->bytes, b->len);
  }));
  close(self->fd);
}

void buffer_view(buffer_t *self) {
  input_dispatch(&self->input, (input_msg_t *)&msg_view, &noop_yield);
}

void buffer_view_lines(buffer_t *self) {
  __auto_type ctx = &self->ctx;
  __auto_type line = self->input.scroll.line;
  char temp[64];
  XGlyphInfo text_size;
  int y = 0;
  draw_set_color(ctx, ctx->palette->secondary_text);
  for(;;line++) {
    sprintf(temp, "%d", line + 1);
    draw_measure_text(ctx, temp, strlen(temp), &text_size);
    draw_text(ctx, self->geometry.lines.w - 12 - text_size.x, y, temp, strlen(temp));
    y += ctx->font->height;
    if (y + ctx->font->height >= self->geometry.lines.h) break;
  }
}

void buffer_get_geometry(buffer_t *self, buffer_geometry_t *geometry) {
  __auto_type ctx = &self->ctx;
  point_t statusbar_size;
  statusbar_measure(&self->statusbar, &statusbar_size);

  geometry->lines.x = 0;
  geometry->lines.y = 0;
  geometry->lines.w = self->show_lines ? 64 : 0;
  geometry->lines.h = ctx->clip.h - statusbar_size.y;

  geometry->textarea.x = geometry->lines.w;
  geometry->textarea.y = 0;
  geometry->textarea.w = ctx->clip.w - geometry->lines.w;
  geometry->textarea.h = ctx->clip.h - statusbar_size.y;

  geometry->statusbar.x = 0;
  geometry->statusbar.y = geometry->textarea.h;
  geometry->statusbar.w = ctx->clip.w;
  geometry->statusbar.h = statusbar_size.y;
}

void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  auto void yield_context_menu(void *msg);
  auto void yield_input(void *msg);

  switch (msg->tag) {
  case Expose: {
    return buffer_view(self);
  }
    case MSG_FREE: {
      buffer_free(self);
      return;
    }
    case MSG_VIEW: {
      buffer_view(self);
      return;
    }
    case MSG_MEASURE: {
      msg->widget.measure->x = INT_MAX;
      msg->widget.measure->y = INT_MAX;
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
      if (msg->context_menu.tag == MSG_VIEW) {
        // SDL_RenderPresent(self->context_menu.ctx.renderer);
      }
      return;
    }
    default: {
      return;
    }
  }
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
