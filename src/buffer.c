#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
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
  self->path = path;
  self->fd = open(path, O_RDWR);
  draw_init_font(&self->font, palette.monospace_font_path, font_size);
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  self->contents = new_bytes(mmaped, st.st_size);

  self->selection.active = BS_INACTIVE;
  bs_index(&self->contents, &self->selection.mark1, 0);
  bs_index(&self->contents, &self->selection.mark2, 60);

  draw_init_context(&self->ctx, &self->font);
  self->scroll.line = 0;
  bs_begin(&self->scroll.pos, &self->contents);
  bs_begin(&self->cursor.pos, &self->contents);
  self->cursor.x0 = 0;
  self->font.font_size = font_size;
  self->_last_command = false;
  self->_prev_keysym = zero_keysym;
  self->command_palette.window = NULL;
  self->command_palette.renderer = NULL;
  self->context_menu.ctx = self->ctx;
  self->context_menu.ctx.window = NULL;
  self->statusbar.ctx = self->ctx;
  statusbar_init(&self->statusbar, self);
  self->lines = NULL;
  self->lines_len = 0;
  self->ibeam_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
}

void buffer_destroy(buffer_t *self) {
  TTF_CloseFont(self->font.font);
  bs_free(self->contents, lambda(void _(bs_bytes_t *b){
    munmap(b->bytes, b->len);
  }));
  if (self->lines) free(self->lines);
  close(self->fd);
  SDL_FreeCursor(self->ibeam_cursor);
}

void buffer_view(buffer_t *self) {
  __auto_type ctx = &self->ctx;
  char temp[1024 * 16]; // Maximum line length — 16kb
  bool inside_selection = false;

  buff_string_iter_t iter = self->scroll.pos;
  int cursor_offset = bs_offset(&self->cursor.pos);
  int mark1_offset = self->selection.active != BS_INACTIVE ? bs_offset(&self->selection.mark1) : -1;
  int mark2_offset
    = self->selection.active == BS_COMPLETE ? bs_offset(&self->selection.mark2)
    : self->selection.active == BS_ONE_MARK ? bs_offset(&self->cursor.pos)
    : self->selection.active == BS_DRAGGING ? bs_offset(&self->cursor.pos)
    : -1;
  if (mark1_offset > mark2_offset) {
    int tmp = mark1_offset;
    mark1_offset = mark2_offset;
    mark2_offset = tmp;
  }

  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  const int cursor_width = 2;

  draw_set_color(ctx, ctx->background);
  SDL_RenderClear(ctx->renderer);

  SDL_RenderSetViewport(ctx->renderer, &self->geometry.lines);
  buffer_view_lines(self, &self->geometry.lines);
  SDL_RenderSetViewport(ctx->renderer, &self->geometry.textarea);

  int y = 0;
  int i = 0;
  for(;;i++) {
    draw_set_color(ctx, ctx->palette->primary_text);
    int offset0 = bs_offset(&iter);
    self->lines[i] = offset0;

    bs_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    int iter_offset = bs_offset(&iter);
    if (cursor_offset >= offset0 && cursor_offset <= iter_offset) {
      draw_set_color(ctx, ctx->palette->current_line_bg);
      draw_box(ctx, 0, y, viewport.w, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette->primary_text);
    }

    if (mark2_offset >= offset0 && mark2_offset <= iter_offset
      && mark1_offset >= offset0 && mark1_offset <= iter_offset) {
      int len = iter_offset - offset0;
      int len1 = mark1_offset - offset0;
      int len2 = mark2_offset - offset0 - len1;
      int len3 = len - (len1 + len2);
      char temp1[len1 + 1];
      char temp2[len2 + 1];
      char temp3[len3 + 1];
      strncpy(temp1, temp, len1);
      temp1[len1]='\0';
      strncpy(temp2, temp + len1, len2);
      temp2[len2]='\0';
      strncpy(temp3, temp + len1 + len2, len3);
      temp3[len3]='\0';
      SDL_Point te1;
      draw_measure_text(ctx, temp1, &te1);
      SDL_Point te2;
      draw_measure_text(ctx, temp2, &te2);
      draw_set_color(ctx, ctx->palette->selection_bg);
      draw_box(ctx, te1.x, y, te2.x, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette->primary_text);
      draw_text(ctx, 0, y, temp1);
      draw_text(ctx, te1.x, y, temp2);
      draw_text(ctx, te1.x + te2.x, y, temp3);
      goto draw_cursor;
    }

    if (mark1_offset >= offset0 && mark1_offset <= iter_offset) {
      int len1 = mark1_offset - offset0;
      int len2 = iter_offset - mark1_offset;
      char temp1[len1 + 1];
      char temp2[len2 + 1];
      strncpy(temp1, temp, len1);
      temp1[len1]='\0';
      strcpy(temp2, temp + len1);
      SDL_Point te1;
      draw_measure_text(ctx, temp1, &te1);
      SDL_Point te2;
      draw_measure_text(ctx, temp2, &te2);

      draw_set_color(ctx, ctx->palette->selection_bg);
      draw_box(ctx, te1.x, y, te2.x, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette->primary_text);
      draw_text(ctx, 0, y, temp1);
      draw_text(ctx, te1.x, y, temp2);
      inside_selection = true;
      goto draw_cursor;
    }

    if (mark2_offset >= offset0 && mark2_offset <= iter_offset) {
      int len1 = mark2_offset - offset0;
      int len2 = iter_offset - mark2_offset;
      char temp1[len1 + 1];
      char temp2[len2 + 1];
      strncpy(temp1, temp, len1);
      temp1[len1]='\0';
      strcpy(temp2, temp + len1);
      SDL_Point te1;
      draw_measure_text(ctx, temp1, &te1);
      SDL_Point te2;
      draw_measure_text(ctx, temp2, &te2);
      draw_set_color(ctx, ctx->palette->selection_bg);
      draw_box(ctx, 0, y, te1.x, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette->primary_text);

      draw_text(ctx, 0, y, temp1);
      draw_text(ctx, te1.x, y, temp2);
      inside_selection = false;
      goto draw_cursor;
    }

    if (*temp != '\0' && *temp != '\n') {
      if (inside_selection) {
        SDL_Point te;
        draw_measure_text(ctx, temp, &te);
        draw_set_color(ctx, ctx->palette->selection_bg);
        draw_box(ctx, 0, y, te.x, ctx->font->X_height);
        draw_set_color(ctx, ctx->palette->primary_text);
      }
      draw_text(ctx, 0, y, temp);
    }
  draw_cursor:
    if (cursor_offset >= offset0 && cursor_offset <= iter_offset) {
      int cur_x_offset = cursor_offset - offset0;
      int cursor_char = temp[cur_x_offset];
      temp[cur_x_offset]='\0';
      SDL_Point te;
      draw_measure_text(ctx, temp, &te);
      SDL_RenderSetViewport(ctx->renderer, NULL);
      draw_box(ctx, self->geometry.textarea.x + te.x - cursor_width / 2, y - 2, cursor_width, ctx->font->X_height + 2);
      SDL_RenderSetViewport(ctx->renderer, &self->geometry.textarea);

      /* if (cursor_char != '\0' && cursor_char != '\n') { */
      /*   temp[0]=cursor_char; */
      /*   temp[1]='\0'; */
      /*   draw_set_color_rgba(ctx, 1, 1, 1, 1); */
      /*   draw_text(ctx, te.x, y, temp); */
      /* } */
    }
    y += ctx->font->X_height;
    if (y + ctx->font->X_height >= self->geometry.textarea.h) break;
    // Skip newline symbol
    bool eof = bs_move(&iter, 1);
    if (eof) break;
  }

  self->lines[i++] = bs_offset(&iter);
  for (;i < self->lines_len; i++) self->lines[i] = -1;

  SDL_RenderSetViewport(ctx->renderer, &self->geometry.statusbar);
  statusbar_view(&self->statusbar);
  SDL_RenderSetViewport(ctx->renderer, &viewport);
}

bool buffer_iter_screen_xy(buffer_t *self, buff_string_iter_t *iter, int screen_x, int screen_y, bool x_adjust) {
  if (self->lines_len < 0) return false;
  __auto_type ctx = &self->ctx;
  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  int x = screen_x - viewport.x;
  int y = screen_y - viewport.y;
  int line_y = div(y, ctx->font->X_height).quot;
  int line_offset = self->lines[line_y];
  if (line_offset < 0) return false;
  int minx, maxx, miny, maxy, advance;

  int current_x = self->geometry.textarea.x;
  bs_index(&self->contents, iter, line_offset);
  bs_find(iter, lambda(bool _(char c){
    if (c == '\n') return true;
    TTF_GlyphMetrics(ctx->font->font, c, &minx, &maxx, &miny, &maxy, &advance);
    current_x += advance;
    if (current_x - (ctx->font->X_width * 0.4) >= x) return true;
    return false;
  }));
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
  int line = self->scroll.line;
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
  geometry->lines.w = 64;
  geometry->lines.h = viewport.h - statusbar_size.y;

  geometry->textarea.x = geometry->lines.w;
  geometry->textarea.y = 0;
  geometry->textarea.w = viewport.w - geometry->lines.w;
  geometry->textarea.h = viewport.h - statusbar_size.y;

  geometry->statusbar.x = 0;
  geometry->statusbar.y = geometry->textarea.h;
  geometry->statusbar.w = viewport.w;
  geometry->statusbar.h = statusbar_size.y;

  int lines_len = div(self->geometry.textarea.h + ctx->font->X_height, ctx->font->X_height).quot + 1;

  if (!self->lines || (lines_len != self->lines_len)) {
    self->lines_len = lines_len;
    self->lines = realloc(self->lines, self->lines_len * sizeof(int));
    if (self->lines_len == 0) self->lines = NULL;
  }
}

static __attribute__((constructor)) void __init__() {
  buffer_widget.update = (update_t)buffer_update;
  buffer_widget.view = (view_t)buffer_view;
  buffer_widget.free = (finalizer_t)buffer_destroy;

  buffer_context_menu_widget.update = (update_t)menulist_update;
  buffer_context_menu_widget.view = (view_t)menulist_view;
  buffer_context_menu_widget.measure = (measure_t)menulist_measure;
  buffer_context_menu_widget.free = (finalizer_t)menulist_free;
}

int buffer_unittest() {
}
