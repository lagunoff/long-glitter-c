#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "buffer.h"
#include "draw.h"
#include "statusbar.h"
#include "main.h"
#include "widget.h"

void buffer_view_lines(buffer_t *self, SDL_Rect *viewport);
static SDL_Keysym zero_keysym = {0};

void buffer_init(buffer_t *self, char *path, int font_size) {
  struct stat st;
  self->fd = open(path, O_RDWR);
  self->path = path;
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  input_init(&self->input, new_bytes(mmaped, st.st_size));
  draw_init_context(&self->ctx, &self->font);
  self->context_menu.ctx = self->ctx;
  self->context_menu.ctx.window = NULL;
  self->statusbar.ctx = self->ctx;
  statusbar_init(&self->statusbar, self);
  self->show_lines = true;
}

void buffer_free(buffer_t *self) {
  input_free(&self->input);
  close(self->fd);
}

void buffer_view(buffer_t *self) {

}

bool buffer_iter_screen_xy(buffer_t *self, buff_string_iter_t *iter, int screen_x, int screen_y, bool x_adjust) {
  /* if (self->lines_len < 0) return false; */
  /* __auto_type ctx = &self->ctx; */
  /* SDL_Rect viewport; */
  /* SDL_RenderGetViewport(ctx->renderer, &viewport); */
  /* int x = screen_x - viewport.x; */
  /* int y = screen_y - viewport.y; */
  /* int line_y = div(y, ctx->font->X_height).quot; */
  /* int line_offset = self->lines[line_y]; */
  /* if (line_offset < 0) return false; */
  /* int minx, maxx, miny, maxy, advance; */

  /* int current_x = self->geometry.textarea.x; */
  /* bs_index(&self->contents, iter, line_offset); */
  /* bs_find(iter, lambda(bool _(char c){ */
  /*   if (c == '\n') return true; */
  /*   TTF_GlyphMetrics(ctx->font->font, c, &minx, &maxx, &miny, &maxy, &advance); */
  /*   current_x += advance; */
  /*   if (current_x - (ctx->font->X_width * 0.4) >= x) return true; */
  /*   return false; */
  /* })); */
  return true;
}

void buffer_view_lines(buffer_t *self, SDL_Rect *viewport) {
  __auto_type ctx = &self->ctx;
  /* draw_set_color(ctx, ctx->palette->hover); */
  /* draw_box(ctx, 0, 0, viewport->w, viewport->h); */
  draw_set_color(ctx, ctx->palette->secondary_text);
  char temp[64];
  SDL_Point text_size;
  int y = 0;
  int line = self->input.scroll.line;
  for(;;line++) {
    sprintf(temp, "%d", line + 1);
    draw_measure_text(ctx, temp, &text_size);
    draw_text(ctx, viewport->w - 12 - text_size.x, y, temp);
    y += ctx->font->X_height;
    if (y + ctx->font->X_height >= viewport->h) break;
  }
}

void buffer_get_geometry(buffer_t *self, buffer_geometry_t *geometry) {
  __auto_type ctx = &self->ctx;
  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  SDL_Point statusbar_size;
  statusbar_measure(&self->statusbar, &statusbar_size);

  geometry->lines.x = 0;
  geometry->lines.y = 0;
  geometry->lines.w = self->show_lines ? 64 : 0;
  geometry->lines.h = viewport.h - statusbar_size.y;

  geometry->textarea.x = geometry->lines.w;
  geometry->textarea.y = 0;
  geometry->textarea.w = viewport.w - geometry->lines.w;
  geometry->textarea.h = viewport.h - statusbar_size.y;

  geometry->statusbar.x = 0;
  geometry->statusbar.y = geometry->textarea.h;
  geometry->statusbar.w = viewport.w;
  geometry->statusbar.h = statusbar_size.y;
}

void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  void yield_context_menu(void *msg) {
    buffer_msg_t buffer_msg = {.tag = BUFFER_CONTEXT_MENU, .context_menu = *(menulist_msg_t *)msg};
    yield(&buffer_msg);
  }

  void yield_input(void *msg) {
    buffer_msg_t buffer_msg = {.tag = BUFFER_INPUT, .input = *(input_msg_t *)msg};
    yield(&buffer_msg);
  }

  switch (msg->tag) {
    case MSG_SDL_EVENT: {
      buffer_dispatch_sdl(self, msg, yield, &yield_context_menu);
      return;
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
      msg->widget.measure.result->x = INT_MAX;
      msg->widget.measure.result->y = INT_MAX;
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
        self->context_menu.ctx.window = NULL;
        return;
      }
      if (msg->context_menu.tag == MENULIST_DESTROY) {
        widget_close_window(self->context_menu.ctx.window);
        self->context_menu.ctx.window = NULL;
        return;
      }
      menulist_dispatch(&self->context_menu, &msg->context_menu, &yield_context_menu);
      if (msg->context_menu.tag == MSG_VIEW) {
        SDL_RenderPresent(self->context_menu.ctx.renderer);
      }
      return;
    }
    default: {
      return;
    }
  }
}

int buffer_unittest() {
}
