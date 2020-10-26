#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "statusbar.h"
#include "main.h"

bool statusbar_update(statusbar_t *self, SDL_Event *e) {
}

void statusbar_view(statusbar_t *self, draw_context_t *ctx) {
  int cursor_offset = bs_offset(&self->cursor->pos);
  char temp[128];
  bool is_saved = self->contents->tag == BS_BYTES;
  sprintf(temp, "Status: %s, Offset: %d, Column: %d", is_saved ? "saved" : "modified", cursor_offset, self->cursor->x0);
  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  SDL_Point fe;
  draw_measure_text(ctx, temp, &fe);

  draw_set_color_rgba(ctx, 0, 0, 0, 0.08);
  draw_box(ctx, 0, 0, viewport.w, 32);

  draw_set_color(ctx, ctx->palette.primary_text);
  draw_text(ctx, 8, (viewport.h - ctx->font->X_height) * 0.5, temp);
}

int statusbar_unittest() {
}
