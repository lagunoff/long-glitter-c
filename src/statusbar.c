#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "statusbar.h"
#include "main.h"

bool statusbar_update(statusbar_t *self, SDL_Event *e) {
}

void statusbar_view(statusbar_t *self, SDL_Renderer *renderer) {
  SDL_Rect rect;
  SDL_Texture *texture1;
  SDL_Color white = {255,255,255,0};
  SDL_Color black = {0,0,0,0};
  SDL_Color bg = white;
  SDL_Color fg = black;
  int cursor_offset = buff_string_offset(&self->cursor->pos);
  char temp[128];
  sprintf(temp, "Cursor: %d, %d", cursor_offset, self->cursor->x0);

  buffer_draw_text(self->font->font, renderer, 0, 0, temp, &texture1, &rect, bg);
  SDL_RenderCopy(renderer, texture1, NULL, &rect);
  SDL_DestroyTexture(texture1);
}

int statusbar_unittest() {
}
