#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "statusbar.h"
#include "main.h"

bool statusbar_update(statusbar_t *self, SDL_Event *e) {
}

void statusbar_view(statusbar_t *self, cairo_t *cr) {
  int cursor_offset = bs_offset(&self->cursor->pos);
  char temp[128];
  bool is_saved = self->contents->tag == BS_BYTES;
  sprintf(temp, "Status: %s, Offset: %d, Column: %d", is_saved ? "saved" : "modified", cursor_offset, self->cursor->x0);
  cairo_font_extents_t fe;
  cairo_font_extents (cr, &fe);
  double x1, x2, y1, y2;
  cairo_clip_extents(cr, &x1, &y1, &x2, &y2);

  cairo_set_source_rgba (cr, 0, 0, 0, 0.08);
  cairo_move_to (cr, 0, 0);
  cairo_rectangle (cr, 0, 0, x2 - x1, 32);
  cairo_fill(cr);

  cairo_set_source_rgba (cr, 0, 0, 0, 0.87);
  cairo_move_to (cr, 0, fe.height);
  cairo_show_text (cr, temp);
}

int statusbar_unittest() {
}
