#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "statusbar.h"
#include "buffer.h"
#include "graphics.h"
#include "utils.h"

static int y_padding = 6;

void statusbar_init(statusbar_t *self, widget_context_t *ctx, struct buffer_t *buffer) {
  self->widget = (widget_basic_t){
    Widget_Basic, {0,0,0,0}, ctx,
    (dispatch_t)&statusbar_dispatch
  };
  self->font = &ctx->palette->small_font;
  self->buffer = buffer;
}

void statusbar_dispatch(statusbar_t *self, statusbar_msg_t *msg, yield_t yield) {
  switch(msg->tag) {
  case Expose: {
    __auto_type ctx = self->widget.ctx;
    __auto_type buff = self->buffer;

    gx_set_color_rgba(ctx, 1 - 0.08, 1 - 0.08, 1 - 0.08, 1);
    gx_rect(ctx, self->widget.clip);
    if (!(self->buffer)) return;

    int cursor_offset = bs_offset(&buff->input.cursor.pos);
    char temp[128];
    char *last_slash = strchr_last(self->buffer->path, '/');
    last_slash = last_slash ? last_slash + 1 : self->buffer->path;
    bool is_saved = buff->input.contents->tag == BuffString_Bytes;
    char sel[64] = {[0] = '\0'};
    __auto_type sel_range = selection_get_range(&buff->input.selection, &buff->input.cursor);
    if (sel_range.x != -1 && sel_range.y != -1) {
      sprintf(sel, " [%d:%d]", sel_range.x, sel_range.y);
    }
    sprintf(temp, "%s (%s), %d:%d%s", last_slash, is_saved ? "saved" : "modified", cursor_offset, buff->input.cursor.x0, sel);

    gx_set_color(ctx, ctx->palette->primary_text);
    gx_text(ctx, self->widget.clip.x + 8, self->widget.clip.y + (self->widget.clip.h - ctx->font->extents.height) * 0.5 + ctx->font->extents.ascent, temp);
  }
  case Widget_Measure: {
    msg->measure.y = self->font->extents.height + y_padding * 2;
    msg->measure.x = INT_MAX;
    return;
  }}
}
