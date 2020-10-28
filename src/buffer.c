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

#define SCROLL_JUMP 8

#define MODIFY_CURSOR(f) {                                        \
  cursor_t old_cursor = self->cursor;                        \
  f(&self->cursor);                                               \
  cursor_modified(self, &old_cursor);                        \
}

static void cursor_modified (
  buffer_t *self,
  cursor_t *prev
);

static SDL_Keysym zero_keysym = {0};

static void buffer_context_menu_init(menulist_t *self);

void buffer_init(buffer_t *self, draw_context_t *ctx, char *path, int font_size) {
  struct stat st;
  self->path = path;
  self->fd = open(path, O_RDWR);
  draw_init_font(&self->font, font_size);
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  self->contents = new_bytes(mmaped, st.st_size);

  self->selection.active = BS_INACTIVE;
  bs_index(&self->contents, &self->selection.mark1, 0);
  bs_index(&self->contents, &self->selection.mark2, 60);

  self->ctx = ctx;
  bs_begin(&self->scroll.pos, &self->contents);
  bs_begin(&self->cursor.pos, &self->contents);
  self->cursor.x0 = 0;
  self->font.font_size = 18;
  self->_last_command = false;
  self->_prev_keysym = zero_keysym;
  self->command_palette.window = NULL;
  self->command_palette.renderer = NULL;
  self->context_menu.ctx = *ctx;
  self->context_menu.ctx.window = NULL;
}

void buffer_destroy(buffer_t *self) {
  TTF_CloseFont(self->font.font);
  bs_free(self->contents, lambda(void _(bs_bytes_t *b){
    munmap(b->bytes, b->len);
  }));
  close(self->fd);
}

void buffer_view(buffer_t *self) {
  draw_context_t *ctx = self->ctx;
  char temp[1024 * 16]; // Maximum line length — 16kb
  bool inside_selection = false;

  buff_string_iter_t iter = self->scroll.pos;
  int y=0;
  int cursor_offset = bs_offset(&self->cursor.pos);
  int mark1_offset = self->selection.active != BS_INACTIVE ? bs_offset(&self->selection.mark1) : -1;
  int mark2_offset
    = self->selection.active == BS_COMPLETE ? bs_offset(&self->selection.mark2)
    : self->selection.active == BS_ONE_MARK ? bs_offset(&self->cursor.pos)
    : -1;
  if (mark1_offset > mark2_offset) {
    int tmp = mark1_offset;
    mark1_offset = mark2_offset;
    mark2_offset = tmp;
  }

  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  const int statusbar_height = ctx->font->X_height + 8;
  const int textarea_height = viewport.h - statusbar_height;
  const int fringe = 8;
  const int cursor_width = 2;

  SDL_Rect textarea_viewport = {fringe, 0, viewport.w - fringe, textarea_height};
  SDL_Rect statusbar_viewport = {0, textarea_height, viewport.w, statusbar_height};
  SDL_RenderSetViewport(ctx->renderer, &textarea_viewport);
  draw_set_color(ctx, ctx->background);
  SDL_RenderClear(ctx->renderer);

  for(;;) {
    draw_set_color(ctx, ctx->palette.primary_text);
    int offset0 = bs_offset(&iter);

    bs_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    int iter_offset = bs_offset(&iter);
    if (cursor_offset >= offset0 && cursor_offset <= iter_offset) {
      draw_set_color(ctx, ctx->palette.current_line_bg);
      draw_box(ctx, 0, y, viewport.w, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette.primary_text);
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
      draw_set_color(ctx, ctx->palette.selection_bg);
      draw_box(ctx, te1.x, y, te2.x, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette.primary_text);
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

      draw_set_color(ctx, ctx->palette.selection_bg);
      draw_box(ctx, te1.x, y, te2.x, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette.primary_text);
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
      draw_set_color(ctx, ctx->palette.selection_bg);
      draw_box(ctx, 0, y, te1.x, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette.primary_text);

      draw_text(ctx, 0, y, temp1);
      draw_text(ctx, te1.x, y, temp2);
      inside_selection = false;
      goto draw_cursor;
    }

    if (*temp != '\0' && *temp != '\n') {
      if (inside_selection) {
        SDL_Point te;
        draw_measure_text(ctx, temp, &te);
        draw_set_color(ctx, ctx->palette.selection_bg);
        draw_box(ctx, 0, y, te.x, ctx->font->X_height);
        draw_set_color(ctx, ctx->palette.primary_text);
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
      draw_box(ctx, fringe + te.x - cursor_width / 2, y - 2, cursor_width, ctx->font->X_height + 2);
      SDL_RenderSetViewport(ctx->renderer, &textarea_viewport);

      /* if (cursor_char != '\0' && cursor_char != '\n') { */
      /*   temp[0]=cursor_char; */
      /*   temp[1]='\0'; */
      /*   draw_set_color_rgba(ctx, 1, 1, 1, 1); */
      /*   draw_text(ctx, te.x, y, temp); */
      /* } */
    }
    y += ctx->font->X_height;
    if (y > viewport.h - statusbar_height) break;
    // Skip newline symbol
    bool eof = bs_move(&iter, 1);
    if (eof) break;
  }

  SDL_RenderSetViewport(ctx->renderer, &statusbar_viewport);
  statusbar_t statusbar = {self};
  statusbar_view(&statusbar, ctx);
}

bool buffer_update(buffer_t *self, SDL_Event *e) {
  if (self->context_menu.ctx.window) {
    menulist_action_t action = menulist_context_update(&self->context_menu, e);
    if (action == MENULIST_DESTROY) {
      draw_close_window(self->context_menu.ctx.window);
      self->context_menu.ctx.window = NULL;
    }
  }
  if (e->type == SDL_MOUSEMOTION) {
    return false;
  }
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    if (e->button.button == SDL_BUTTON_RIGHT) {
      if (self->context_menu.ctx.window) draw_close_window(self->context_menu.ctx.window);
      SDL_Point size_pos;
      SDL_GetWindowPosition(self->ctx->window, &size_pos.x, &size_pos.y);
      size_pos.x += e->button.x;
      size_pos.y += e->button.y;
      buffer_context_menu_init(&self->context_menu);
      draw_open_window_measure(&size_pos, SDL_WINDOW_TOOLTIP, &self->context_menu.ctx.window, &self->context_menu.ctx.renderer, &buffer_context_menu_widget, &self->context_menu);
    }
    goto continue_command;
  }
  if (e->type == SDL_KEYDOWN) {
    if (e->key.keysym.scancode == SDL_SCANCODE_X && (e->key.keysym.mod & KMOD_CTRL)) {
      self->_prev_keysym = e->key.keysym;
      goto continue_dont_clear_prev;
    }

    if (e->key.keysym.scancode == SDL_SCANCODE_S && (e->key.keysym.mod & KMOD_CTRL)
        && self->_prev_keysym.scancode == SDL_SCANCODE_X && (self->_prev_keysym.mod & KMOD_CTRL)) {
      buff_string_iter_t iter;
      bs_begin(&iter, &self->contents);
      int buf_len = 1024 * 64;
      char buf[buf_len];
      lseek(self->fd, 0, SEEK_SET);
      for (;;) {
        int taken = bs_take_2(&iter, buf, buf_len);
        int written = 0;
        for(;;) {
          int w = write(self->fd, buf + written, taken - written);
          // TODO: handle write error
          if (w == -1) goto continue_command;
          written += w;
          if (written >= taken) break;
        }
        if (taken < buf_len) break;
      }
      draw_context_t *ctx = self->ctx;
      char *path = self->path;
      int font_size = self->font.font_size;
      buffer_destroy(self);
      buffer_init(self, ctx, path, font_size);
      goto continue_command;
    }

    if (e->key.keysym.scancode == SDL_SCANCODE_LEFT
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
      SDL_Rect viewport;
      SDL_RenderGetViewport(self->ctx->renderer, &viewport);
      scroll_page(&self->scroll, &self->cursor, &self->font, viewport.h, 1);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_LGUI)) {
      debug0("debuggggggg");
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_X && (e->key.keysym.mod & KMOD_ALT)) {
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_Y && (e->key.keysym.mod & KMOD_CTRL)) {
      char *clipboard = SDL_GetClipboardText();
      if (clipboard) {
        self->contents = bs_insert(
          self->contents,
          self->cursor.pos.global_index,
          clipboard, 0, BS_LEFT,
          &self->cursor.pos,
          &self->scroll.pos, NULL
        );
      }
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_K && (e->key.keysym.mod & KMOD_CTRL)) {
      buff_string_iter_t iter = self->cursor.pos;
      bs_find(&iter, lambda(bool _(char c) { return c == '\n'; }));
      int curr_offset = bs_offset(&self->cursor.pos);
      int iter_offset = bs_offset(&iter);
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", MAX(iter_offset - curr_offset, 1),
        BS_RIGHT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_BACKSPACE && (e->key.keysym.mod & KMOD_ALT)) {
      buff_string_iter_t iter = self->cursor.pos;
      bs_backward_word(&iter);

      int curr_offset = bs_offset(&self->cursor.pos);
      int iter_offset = bs_offset(&iter);
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", MAX(curr_offset - iter_offset, 1),
        BS_LEFT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_ALT)) {
      buff_string_iter_t iter = self->cursor.pos;
      bs_forward_word(&iter);

      int curr_offset = bs_offset(&self->cursor.pos);
      int iter_offset = bs_offset(&iter);
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", MAX(iter_offset - curr_offset, 1),
        BS_RIGHT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_V && (e->key.keysym.mod & KMOD_ALT)
        || e->key.keysym.scancode == SDL_SCANCODE_PAGEUP && (e->key.keysym.mod==0)
        ) {
      SDL_Rect viewport;
      SDL_RenderGetViewport(self->ctx->renderer, &viewport);
      scroll_page(&self->scroll, &self->cursor, &self->font, viewport.h, -1);
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
      self->font.font_size += 1;
      draw_init_font(&self->font, self->font.font_size);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_MINUS && (e->key.keysym.mod & KMOD_CTRL)) {
      TTF_CloseFont(self->font.font);
      self->font.font_size -= 1;
      draw_init_font(&self->font, self->font.font_size);
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
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", 1,
        BS_RIGHT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_BACKSPACE && (e->key.keysym.mod == 0)) {
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", 1,
        BS_LEFT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RETURN) {
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "\n", 0,
        BS_LEFT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      goto continue_no_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_SLASH && (e->key.keysym.mod & KMOD_CTRL)) {
      self->contents = bs_insert_undo(self->contents, &self->cursor.pos, &self->scroll.pos, NULL);
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_SPACE && (e->key.keysym.mod & KMOD_CTRL)) {
      self->selection.active = BS_ONE_MARK;
      self->selection.mark1 = self->cursor.pos;
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_G && (e->key.keysym.mod & KMOD_CTRL)) {
      self->selection.active = BS_INACTIVE;
      self->selection.mark1 = self->cursor.pos;
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_W && (e->key.keysym.mod & KMOD_CTRL)) {
      if (self->selection.active) {
        buff_string_iter_t mark_1 = self->selection.mark1;
        buff_string_iter_t mark_2 = self->cursor.pos;
        if (mark_1.global_index > mark_2.global_index) {
          buff_string_iter_t tmp = mark_1;
          mark_1 = mark_2;
          mark_2 = tmp;
        }
        int len = mark_2.global_index - mark_1.global_index;
        char temp[len + 1];
        buff_string_iter_t mark_1_temp = mark_1;
        bs_take(&mark_1_temp, temp, len);
        SDL_SetClipboardText(temp);
        self->contents = bs_insert(
          self->contents,
          mark_1.global_index,
          "", len,
          BS_RIGHT,
          &self->cursor.pos,
          &self->scroll.pos, NULL
        );
        self->selection.active = BS_INACTIVE;
      }
      goto continue_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_W && (e->key.keysym.mod & KMOD_ALT)) {
      if (self->selection.active) {
        buff_string_iter_t mark_1 = self->selection.mark1;
        buff_string_iter_t mark_2 = self->cursor.pos;
        if (mark_1.global_index > mark_2.global_index) {
          buff_string_iter_t tmp = mark_1;
          mark_1 = mark_2;
          mark_2 = tmp;
        }
        int len = mark_2.global_index - mark_1.global_index;
        char temp[len + 1];
        bs_take(&mark_1, temp, len);
        SDL_SetClipboardText(temp);
        self->selection.active = BS_INACTIVE;
      }
      goto continue_command;
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
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        e->text.text, 0,
        BS_LEFT,
        &self->cursor.pos,
        &self->scroll.pos, NULL
      );
      return true;
    }
  }
  return false;
}

void buffer_init_widget(widget_t *self, buffer_t *model) {
}

static void cursor_modified (
  buffer_t *self,
  cursor_t *prev
) {
  SDL_Rect viewport;
  SDL_RenderGetViewport(self->ctx->renderer, &viewport);
  int next_offset = bs_offset(&self->cursor.pos);
  int prev_offset = bs_offset(&prev->pos);
  int scroll_offset = bs_offset(&self->scroll.pos);
  int diff = next_offset - prev_offset;
  int screen_lines = div(viewport.h, self->font.X_height).quot;

  int count_lines() {
    buff_string_iter_t iter = self->cursor.pos;
    int lines=0;
    int i = next_offset;
    if (next_offset > scroll_offset) {
      bs_find_back(&iter, lambda(bool _(char c) {
        if (c=='\n') lines++;
        i--;
        if (i==scroll_offset) return true;
        return false;
      }));
    } else {
      bs_find(&iter, lambda(bool _(char c) {
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

static menulist_item_t items[] = {
  {.title="Cut"}, {.title="Copy"}, {.title="Paste"}
};

static void
buffer_context_menu_init(menulist_t *self) {
  menulist_init(self, items, sizeof(items)/sizeof(menulist_item_t), sizeof(menulist_item_t));
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
