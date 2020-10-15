#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "buffer.h"
#include "main.h"

#define SCROLL_JUMP 8

#define MODIFY_CURSOR(f) {                                        \
  struct cursor old_cursor = self->cursor;                        \
  f(&self->cursor);                                               \
  cursor_modified(self, &old_cursor);                             \
}

static void buffer_init_font(struct loaded_font *out, int font_size);

static void buffer_draw_text(
  TTF_Font *font,
  SDL_Renderer *renderer,
  int x, int y,
  char *text,
  SDL_Texture **out_texture,
  SDL_Rect *out_rect,
  SDL_Color bg
);

static void cursor_modified (
  struct buffer *self,
  struct cursor *prev
);

static SDL_Keysym zero_keysym = {0};

void buffer_init(struct buffer *out, SDL_Point *size, char *path) {
  struct stat st;
  out->fd = open(path, O_RDWR);
  fstat(out->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, out->fd, 0);
  char *contents = malloc(st.st_size);
  memcpy(contents, mmaped, st.st_size);
  munmap(mmaped, st.st_size);
  out->contents.bytes = contents;
  out->contents.len = st.st_size;
  out->contents.first = NULL;
  out->contents.last = NULL;
  out->selection.active = false;
  buff_string_index(&out->contents, &out->selection.mark1, 10);
  buff_string_index(&out->contents, &out->selection.mark2, 20);

  out->size = *size;
  buff_string_begin(&out->scroll.pos, &out->contents);
  buff_string_begin(&out->cursor.pos, &out->contents);
  out->cursor.x0 = 0;
  buffer_init_font(&out->font, 18);
  out->_last_command = false;
  out->_prev_keysym = zero_keysym;
}

void buffer_destroy(struct buffer *self) {
  TTF_CloseFont(self->font.font);
}

bool buffer_update(struct buffer *self, SDL_Event *e) {
  if (e->type == SDL_KEYDOWN) {
    if (e->key.keysym.scancode == SDL_SCANCODE_X && (e->key.keysym.mod & KMOD_CTRL)) {
      debug0("prev = e->key");
      self->_prev_keysym = e->key.keysym;
      goto continue_dont_clear_prev;
    }

    if (e->key.keysym.scancode == SDL_SCANCODE_S && (e->key.keysym.mod & KMOD_CTRL)
        && self->_prev_keysym.scancode == SDL_SCANCODE_X && (self->_prev_keysym.mod & KMOD_CTRL)) {
      int written = 0;
      for (;;) {
        int w = write (self->fd, self->contents.bytes + written, self->contents.len - written);
        // TODO: handle write error
        if (w == -1) goto continue_command;
        written += w;
        if (written >= self->contents.len) break;
      }
      goto continue_command;
    }

    if ( e->key.keysym.scancode == SDL_SCANCODE_LEFT
         || e->key.keysym.scancode == SDL_SCANCODE_B && (e->key.keysym.mod & KMOD_CTRL)
         ) {
      MODIFY_CURSOR(cursor_left);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RIGHT
        || e->key.keysym.scancode == SDL_SCANCODE_F && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_right);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_DOWN && e->key.keysym.mod == 0
        || e->key.keysym.scancode == SDL_SCANCODE_N && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_down);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_UP && e->key.keysym.mod == 0
        || e->key.keysym.scancode == SDL_SCANCODE_P && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_up);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_F && (e->key.keysym.mod & KMOD_ALT)) {
      MODIFY_CURSOR(cursor_forward_word);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_B && (e->key.keysym.mod & KMOD_ALT)) {
      MODIFY_CURSOR(cursor_backward_word);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_E && (e->key.keysym.mod & KMOD_CTRL)) {
      MODIFY_CURSOR(cursor_eol);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_A && (e->key.keysym.mod & KMOD_CTRL)) {
      MODIFY_CURSOR(cursor_bol);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_V && (e->key.keysym.mod & KMOD_CTRL)
        || e->key.keysym.scancode == SDL_SCANCODE_PAGEDOWN && (e->key.keysym.mod==0)
        ) {
      scroll_page(&self->scroll, &self->cursor, &self->font, self->size.y, 1);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_Y && (e->key.keysym.mod & KMOD_CTRL)) {
      char *clipboard = SDL_GetClipboardText();
      if (clipboard) {
        buff_string_insert(&self->cursor.pos, INSERT_LEFT, clipboard, 0, &self->scroll.pos, NULL);
      }
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_K && (e->key.keysym.mod & KMOD_CTRL)) {
      struct buff_string_iter iter = self->cursor.pos;
      buff_string_find(&iter, lambda(bool _(char c) { return c == '\n'; }));
      int curr_offset = buff_string_offset(&self->cursor.pos);
      int iter_offset = buff_string_offset(&iter);
      buff_string_insert(&self->cursor.pos, INSERT_RIGHT, "", MAX(iter_offset - curr_offset, 1), &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_BACKSPACE && (e->key.keysym.mod & KMOD_ALT)) {
      struct buff_string_iter iter = self->cursor.pos;
      buff_string_backward_word(&iter);

      int curr_offset = buff_string_offset(&self->cursor.pos);
      int iter_offset = buff_string_offset(&iter);
      buff_string_insert(&self->cursor.pos, INSERT_LEFT, "", abs(curr_offset - iter_offset), &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_ALT)) {
      struct buff_string_iter iter = self->cursor.pos;
      buff_string_forward_word(&iter);

      int curr_offset = buff_string_offset(&self->cursor.pos);
      int iter_offset = buff_string_offset(&iter);
      buff_string_insert(&self->cursor.pos, INSERT_RIGHT, "", abs(curr_offset - iter_offset), &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_V && (e->key.keysym.mod & KMOD_ALT)
        || e->key.keysym.scancode == SDL_SCANCODE_PAGEUP && (e->key.keysym.mod==0)
        ) {
      scroll_page(&self->scroll, &self->cursor, &self->font, self->size.y, -1);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(backward_paragraph);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(forward_paragraph);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_EQUALS  && (e->key.keysym.mod & KMOD_CTRL)) {
      TTF_CloseFont(self->font.font);
      buffer_init_font(&self->font, self->font.font_size + 1);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_MINUS && (e->key.keysym.mod & KMOD_CTRL)) {
      TTF_CloseFont(self->font.font);
      buffer_init_font(&self->font, self->font.font_size - 1);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_PERIOD && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(cursor_end);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_COMMA && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(cursor_begin);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_DELETE && (e->key.keysym.mod == 0)
        || e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      buff_string_insert(&self->cursor.pos, INSERT_RIGHT, "", 1, &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_BACKSPACE && (e->key.keysym.mod == 0)) {
      buff_string_insert(&self->cursor.pos, INSERT_LEFT, "", 1, &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RETURN) {
      buff_string_insert(&self->cursor.pos, INSERT_LEFT, "\n", 0, &self->scroll.pos, NULL);
      goto continue_no_command;
    }
    self->_last_command = false;
    return false;

  continue_no_command:
    self->_last_command = false;
    self->_prev_keysym = zero_keysym;
    return true;

  continue_command:
    self->_last_command = true;
    self->_prev_keysym = zero_keysym;
    return true;

  continue_dont_clear_prev:
    self->_last_command = false;
    return true;

  continue_dont_render:
    self->_last_command = false;
    return false;
  }

  if (e->type == SDL_TEXTINPUT) {
    if (!self->_last_command) {
      buff_string_insert(&self->cursor.pos, INSERT_LEFT, e->text.text, 0, &self->scroll.pos, NULL);
      return true;
    }
  }
  return false;
}

void buffer_view(struct buffer *self, SDL_Renderer *renderer) {
  SDL_Rect rect;
  SDL_Texture *texture1;
  char temp[1024 * 16]; // Maximum line length — 16kb
  struct buff_string_iter iter = self->scroll.pos;
  int y=0;
  int cursor_offset = buff_string_offset(&self->cursor.pos);
  int mark1_offset = self->selection.active ? buff_string_offset(&self->selection.mark1) : -1;
  int mark2_offset = self->selection.active ? buff_string_offset(&self->selection.mark2) : -1;
  SDL_Color white = {255,255,255,0};
  SDL_Color black = {0,0,0,0};
  SDL_Color selection_bg = {210,210,255,0};

  SDL_Color bg = white;
  SDL_Color fg = black;

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
  SDL_RenderClear(renderer);

  for(;;) {
    SDL_SetRenderDrawColor(renderer, fg.r, fg.g, fg.b, fg.a);
    int offset0 = buff_string_offset(&iter);

    buff_string_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    int iter_offset = buff_string_offset(&iter);

    if (mark2_offset >= offset0 && mark2_offset <= iter_offset) {
      bg = white;
    }

    if (*temp != '\0' && *temp != '\n') {
      buffer_draw_text(self->font.font, renderer, 0, y, temp, &texture1, &rect, bg);
      SDL_RenderCopy(renderer, texture1, NULL, &rect);
      SDL_DestroyTexture(texture1);
    }


    if (mark1_offset >= offset0 && mark1_offset <= iter_offset) {
      int w,h;
      bg = selection_bg;
      int len1 = mark1_offset - offset0;
      int len2 = iter_offset - mark1_offset;
      char temp1[len1 + 1];
      char temp2[len2 + 1];
      strncpy(temp1, temp, len1);
      temp1[len1]='\0';
      strcpy(temp2, temp + len1);
      TTF_SizeText(self->font.font,temp1,&w,&h);
      buffer_draw_text(self->font.font, renderer, w, y, temp2, &texture1, &rect, bg);
      SDL_RenderCopy(renderer, texture1, NULL, &rect);
      SDL_DestroyTexture(texture1);
    }

    if (mark2_offset >= offset0 && mark2_offset <= iter_offset) {
      int len1 = mark2_offset - offset0;
      char temp1[len1 + 1];
      strncpy(temp1, temp, len1);
      temp1[len1]='\0';
      buffer_draw_text(self->font.font, renderer, 0, y, temp1, &texture1, &rect, selection_bg);
      SDL_RenderCopy(renderer, texture1, NULL, &rect);
      SDL_DestroyTexture(texture1);
    }

    if (cursor_offset >= offset0 && cursor_offset <= iter_offset) {
      int w,h;
      int cur_x_offset = cursor_offset - offset0;
      int cursor_char = temp[cur_x_offset];
      temp[cur_x_offset]='\0';
      TTF_SizeText(self->font.font,temp,&w,&h);
      if (cur_x_offset == 0) w=0;
      SDL_Rect cursor_rect = {x:w,y:y,w:self->font.X_width,h:self->font.X_height};
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
      SDL_RenderFillRect(renderer, &cursor_rect);
      if (cursor_char != '\0' && cursor_char != '\n') {
        temp[0]=cursor_char;
        temp[1]='\0';
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
        buffer_draw_text(self->font.font, renderer, w, y, temp, &texture1, &rect, black);
        SDL_RenderCopy(renderer, texture1, NULL, &rect);
        SDL_DestroyTexture(texture1);
      }
    }

    y += self->font.X_height;
    if (y > self->size.y) break;
    // Skip newline symbol
    buff_string_move(&iter, 1);
  }

  SDL_RenderPresent(renderer);
}

void cursor_modified (
  struct buffer *self,
  struct cursor *prev
) {
  int next_offset = buff_string_offset(&self->cursor.pos);
  int prev_offset = buff_string_offset(&prev->pos);
  int scroll_offset = buff_string_offset(&self->scroll.pos);
  int diff = next_offset - prev_offset;
  int screen_lines = div(self->size.y, self->font.X_height).quot;

  int count_lines() {
    struct buff_string_iter iter = self->cursor.pos;
    int lines=0;
    int i = next_offset;
    if (next_offset > scroll_offset) {
      buff_string_find_back(&iter, lambda(bool _(char c) {
        if (c=='\n') lines++;
        i--;
        if (i==scroll_offset) return true;
        return false;
      }));
    } else {
      buff_string_find(&iter, lambda(bool _(char c) {
        if (c=='\n') lines--;
        i++;
        if (i==scroll_offset) return true;
        return false;
      }));
    }
    return lines;
  }
  int lines = count_lines();

  if (diff > 0) {
    if (lines > screen_lines) {
      scroll_lines(&self->scroll, lines - screen_lines + SCROLL_JUMP);
    }
  } else {
    if (lines < 0) {
      scroll_lines(&self->scroll, lines - SCROLL_JUMP - 1);
    }
  }
}

void buffer_init_font(struct loaded_font *out, int font_size) {
  out->font_size = font_size;
  out->font = TTF_OpenFont("/home/vlad/downloads/ttf/Hack-Regular.ttf", font_size);
  TTF_SizeText(out->font, "X", &out->X_width, &out->X_height);
  if (out->font == NULL) {
    fprintf(stderr, "error: font not found\n");
    exit(EXIT_FAILURE);
  }
}

void buffer_draw_text(
  TTF_Font *font,
  SDL_Renderer *renderer,
  int x, int y,
  char *text,
  SDL_Texture **out_texture,
  SDL_Rect *out_rect,
  SDL_Color bg
) {
  int text_width;
  int text_height;
  SDL_Color fg;
  SDL_GetRenderDrawColor(renderer, &fg.r, &fg.g, &fg.b, &fg.a);
  SDL_Surface *surface = TTF_RenderText_Shaded(font, text, fg, bg);
  *out_texture = SDL_CreateTextureFromSurface(renderer, surface);
  text_width = surface->w;
  text_height = surface->h;
  SDL_FreeSurface(surface);
  out_rect->x = x;
  out_rect->y = y;
  out_rect->w = text_width;
  out_rect->h = text_height;
}

int buffer_unittest() {
}
