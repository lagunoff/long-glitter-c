#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cairo.h>

#include "buffer.h"
#include "statusbar.h"
#include "main.h"

#define SCROLL_JUMP 8

#define MODIFY_CURSOR(f) {                                        \
  struct cursor old_cursor = self->cursor;                        \
  f(&self->cursor);                                               \
  cursor_modified(self, &old_cursor);                             \
}

static void buffer_init_font(struct loaded_font *out, int font_size);

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
  out->contents.bytes = mmaped;
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
  munmap(self->contents.bytes, self->contents.len);
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
    if (e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_LGUI)) {
      debug0("debuggggggg");
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_W && (e->key.keysym.mod & KMOD_LGUI)) {
      SDL_Renderer *renderer;
      SDL_Window *tooltip;
      debug0("Creating a window!");

      if (SDL_CreateWindowAndRenderer(240, 360, SDL_WINDOW_TOOLTIP, &tooltip, &renderer) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        exit (EXIT_FAILURE);
      }
      debug0("Creating a window!");
      SDL_ShowWindow(tooltip);
      // SDL_RaiseWindow(window);
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
      SDL_RenderClear(renderer);
      //      SDL_Rect bg = {x:w,y:y,w:self->font.X_width,h:self->font.X_height};

      SDL_RenderPresent(renderer);

      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_Y && (e->key.keysym.mod & KMOD_CTRL)) {
      char *clipboard = SDL_GetClipboardText();
      if (clipboard) {
        buff_string_insert(&self->cursor.pos, BSD_LEFT, clipboard, 0, &self->scroll.pos, NULL);
      }
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_K && (e->key.keysym.mod & KMOD_CTRL)) {
      struct buff_string_iter iter = self->cursor.pos;
      buff_string_find(&iter, lambda(bool _(char c) { return c == '\n'; }));
      int curr_offset = buff_string_offset(&self->cursor.pos);
      int iter_offset = buff_string_offset(&iter);
      buff_string_insert(&self->cursor.pos, BSD_RIGHT, "", MAX(iter_offset - curr_offset, 1), &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_BACKSPACE && (e->key.keysym.mod & KMOD_ALT)) {
      struct buff_string_iter iter = self->cursor.pos;
      buff_string_backward_word(&iter);

      int curr_offset = buff_string_offset(&self->cursor.pos);
      int iter_offset = buff_string_offset(&iter);
      buff_string_insert(&self->cursor.pos, BSD_LEFT, "", abs(curr_offset - iter_offset), &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_ALT)) {
      struct buff_string_iter iter = self->cursor.pos;
      buff_string_forward_word(&iter);

      int curr_offset = buff_string_offset(&self->cursor.pos);
      int iter_offset = buff_string_offset(&iter);
      buff_string_insert(&self->cursor.pos, BSD_RIGHT, "", abs(curr_offset - iter_offset), &self->scroll.pos, NULL);
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
      buff_string_insert(&self->cursor.pos, BSD_RIGHT, "", 1, &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_BACKSPACE && (e->key.keysym.mod == 0)) {
      buff_string_insert(&self->cursor.pos, BSD_LEFT, "", 1, &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RETURN) {
      buff_string_insert(&self->cursor.pos, BSD_LEFT, "\n", 0, &self->scroll.pos, NULL);
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
      buff_string_insert(&self->cursor.pos, BSD_LEFT, e->text.text, 0, &self->scroll.pos, NULL);
      return true;
    }
  }
  return false;
}

void buffer_view(struct buffer *self, cairo_t *cr) {
  char temp[1024 * 16]; // Maximum line length — 16kb
  struct buff_string_iter iter = self->scroll.pos;
  int y=0;
  int cursor_offset = buff_string_offset(&self->cursor.pos);
  int mark1_offset = self->selection.active ? buff_string_offset(&self->selection.mark1) : -1;
  int mark2_offset = self->selection.active ? buff_string_offset(&self->selection.mark2) : -1;
  const int statusbar_height = 32;
  const int textarea_height = self->size.y - statusbar_height;

  cairo_select_font_face (cr, "Hack", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 18);
  cairo_font_extents_t fe;
  cairo_font_extents (cr, &fe);

  for(;;) {
    cairo_set_source_rgba (cr, 0, 0, 0, 0.87);
    int offset0 = buff_string_offset(&iter);

    buff_string_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    int iter_offset = buff_string_offset(&iter);

    if (*temp != '\0' && *temp != '\n') {
      cairo_move_to (cr, 0, y + fe.ascent);
      cairo_show_text (cr, temp);
    }

    if (cursor_offset >= offset0 && cursor_offset <= iter_offset) {
      int w,h;
      int cur_x_offset = cursor_offset - offset0;
      int cursor_char = temp[cur_x_offset];
      temp[cur_x_offset]='\0';
      cairo_text_extents_t te;
      cairo_text_extents (cr, temp, &te);
      w = te.x_advance;
      if (cur_x_offset == 0) w=0;

      cairo_rectangle(cr, w, y, fe.max_x_advance, fe.height);
      cairo_fill(cr);

      if (cursor_char != '\0' && cursor_char != '\n') {
        temp[0]=cursor_char;
        temp[1]='\0';
        cairo_set_source_rgb (cr, 1, 1, 1);
        cairo_move_to (cr, w, y + fe.ascent);
        cairo_show_text (cr, temp);
      }
    }

    y += fe.height;
    if (y > self->size.y) break;
    // Skip newline symbol
    bool eof = buff_string_move(&iter, 1);
    if (eof) break;
  }

  cairo_matrix_t matrix;
  cairo_get_matrix(cr, &matrix);
  cairo_translate(cr, 0, self->size.y - statusbar_height);
  statusbar_t statusbar = {&self->font, &self->cursor};
  statusbar_view(&statusbar, cr);
  cairo_set_matrix(cr, &matrix);
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

int buffer_unittest() {
}
