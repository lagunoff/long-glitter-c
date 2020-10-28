#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "statusbar.h"
#include "buffer.h"
#include "draw.h"
#include "main.h"

static char *strchr_last(char *str, int c);
static int y_padding = 6;

void statusbar_init(statusbar_t *self, struct buffer_t *buffer) {
  self->ctx.font = &palette.small_font;
  self->buffer = buffer;
}

bool statusbar_update(statusbar_t *self, SDL_Event *e) {
  return false;
}

void statusbar_measure(statusbar_t *self, SDL_Point *size) {
  size->y = self->ctx.font->X_height + y_padding * 2;
  size->x = INT_MAX;
}

void statusbar_view(statusbar_t *self) {
  draw_context_t *ctx = &self->ctx;
  int cursor_offset = bs_offset(&self->buffer->cursor.pos);
  char temp[128];
  char *last_slash = strchr_last(self->buffer->path, '/');
  last_slash = last_slash ? last_slash + 1 : self->buffer->path;
  bool is_saved = self->buffer->contents->tag == BS_BYTES;
  sprintf(temp, "%s (%s), %d:%d", last_slash, is_saved ? "saved" : "modified", cursor_offset, self->buffer->cursor.x0);
  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);

  draw_set_color_rgba(ctx, 0, 0, 0, 0.08);
  draw_box(ctx, 0, 0, viewport.w, viewport.h);

  draw_set_color(ctx, ctx->palette->primary_text);
  draw_text(ctx, 8, (viewport.h - ctx->font->X_height) * 0.5, temp);
}

static char *strchr_last(char *str, int c) {
  char *result = NULL;
  for(int i = 0; str[i]; i++)
    if (str[i] == c) result = str + i;
  return result;
}

int statusbar_unittest() {
}
