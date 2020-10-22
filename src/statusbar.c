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
  int cursor_offset = buff_string_offset(&self->cursor->pos);
  char temp[128];
  sprintf(temp, "Cursor: %d, %d", cursor_offset, self->cursor->x0);
  cairo_set_source_rgba (cr, 0, 0, 0, 0.87);
  cairo_font_extents_t fe;
  cairo_font_extents (cr, &fe);

  cairo_move_to (cr, 0, fe.ascent);
  cairo_show_text (cr, temp);
}

int statusbar_unittest() {
}
