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

static int y_padding = 6;

void statusbar_init(statusbar_t *self, widget_context_init_t *ctx, struct buffer_t *buffer) {
  draw_init_context(&self->ctx, ctx);
  self->ctx.font = &self->ctx.palette->small_font;
  self->buffer = buffer;
}

void statusbar_dispatch(statusbar_t *self, statusbar_msg_t *msg, yield_t yield) {
  switch(msg->tag) {
  case Expose: {
    __auto_type ctx = &self->ctx;
    __auto_type buff = self->buffer;

    draw_set_color_rgba(ctx, 1 - 0.08, 1 - 0.08, 1 - 0.08, 1);
    draw_rect(ctx, ctx->clip);
    if (!(self->buffer)) return;

    int cursor_offset = bs_offset(&self->buffer->input.cursor.pos);
    char temp[128];
    char *last_slash = strchr_last(self->buffer->path, '/');
    last_slash = last_slash ? last_slash + 1 : self->buffer->path;
    bool is_saved = self->buffer->input.contents->tag == BuffString_Bytes;
    char sel[64] = {[0] = '\0'};
    __auto_type sel_range = selection_get_range(&buff->input.selection, &buff->input.cursor);
    if (sel_range.x != -1 && sel_range.y != -1) {
      sprintf(sel, " [%d:%d]", sel_range.x, sel_range.y);
    }
    sprintf(temp, "%s (%s), %d:%d%s", last_slash, is_saved ? "saved" : "modified", cursor_offset, self->buffer->input.cursor.x0, sel);

    draw_set_color(ctx, ctx->palette->primary_text);
    draw_text(ctx, ctx->clip.x + 8, ctx->clip.y + (ctx->clip.h - ctx->font->extents.height) * 0.5 + ctx->font->extents.ascent, temp);
  }
  case Widget_Measure: {
    msg->measure.y = self->ctx.font->extents.height + y_padding * 2;
    msg->measure.x = INT_MAX;
    return;
  }}
}
