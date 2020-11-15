#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "statusbar.h"
#include "buffer.h"
#include "draw.h"
#include "utils.h"

static char *strchr_last(char *str, int c);
static int y_padding = 6;

void statusbar_init(statusbar_t *self, struct buffer_t *buffer) {
  self->ctx.font = palette.small_font;
  self->buffer = buffer;
}

void statusbar_measure(statusbar_t *self, point_t *size) {
  size->y = self->ctx.font->height + y_padding * 2;
  size->x = INT_MAX;
}

void statusbar_view(statusbar_t *self) {
  __auto_type ctx = &self->ctx;
  __auto_type buff = self->buffer;
  int cursor_offset = bs_offset(&self->buffer->input.cursor.pos);
  char temp[128];
  char *last_slash = strchr_last(self->buffer->path, '/');
  last_slash = last_slash ? last_slash + 1 : self->buffer->path;
  bool is_saved = self->buffer->input.contents->tag == BS_BYTES;
  char sel[64] = {[0] = '\0'};
  __auto_type sel_range = selection_get_range(&buff->input.selection, &buff->input.cursor);
  if (sel_range.x != -1 && sel_range.y != -1) {
    sprintf(sel, " [%d:%d]", sel_range.x, sel_range.y);
  }
  sprintf(temp, "%s (%s), %d:%d%s", last_slash, is_saved ? "saved" : "modified", cursor_offset, self->buffer->input.cursor.x0, sel);

  draw_set_color_rgba(ctx, 0, 0, 0, 0.08);
  draw_box(ctx, 0, 0, ctx->clip.w, ctx->clip.h);

  draw_set_color(ctx, ctx->palette->primary_text);
  draw_text(ctx, 8, (ctx->clip.h - ctx->font->height) * 0.5, temp, strlen(temp));
}

static char *strchr_last(char *str, int c) {
  char *result = NULL;
  for(int i = 0; str[i]; i++)
    if (str[i] == c) result = str + i;
  return result;
}

int statusbar_unittest() {
}
