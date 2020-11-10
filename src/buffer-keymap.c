#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "buffer.h"
#include "main.h"

#define SCROLL_JUMP 8
#define MODIFY_CURSOR(f) {                                        \
  cursor_t old_cursor = self->cursor;                        \
  f(&self->cursor);                                               \
  cursor_modified(self, &old_cursor);                        \
}

static void cursor_modified (buffer_t *self,cursor_t *prev);
static void buffer_context_menu_init(menulist_t *self);
static SDL_Keysym zero_keysym = {0};
static void buffer_context_menu_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield);

void buffer_dispatch_sdl(buffer_t *self, buffer_msg_t *msg, yield_t yield, yield_t yield_cm) {
  if (msg->tag != MSG_SDL_EVENT) goto _noop;

  __auto_type e = msg->widget.sdl_event.event;
  if (self->context_menu.ctx.window) {
    menulist_msg_t msg = menulist_parent_window(e);
    menulist_dispatch(&self->context_menu, &msg, yield_cm);
  }
  if (e->type == SDL_MOUSEMOTION) {
    if (self->selection.state == SELECTION_DRAGGING_MOUSE) {
      buffer_iter_screen_xy(self, &self->cursor.pos, e->motion.x, e->motion.y, true);
      goto _view;
    }
    goto _noop;
  }
  if (e->type == SDL_MOUSEWHEEL) {
    scroll_lines(&self->scroll, -e->wheel.y);
    goto _view;
  }
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    if (e->button.button == SDL_BUTTON_RIGHT) {
      if (self->context_menu.ctx.window) widget_close_window(self->context_menu.ctx.window);
      SDL_Point size_pos;
      SDL_GetWindowPosition(self->ctx.window, &size_pos.x, &size_pos.y);
      size_pos.x += e->button.x;
      size_pos.y += e->button.y;
      buffer_context_menu_init(&self->context_menu);
      widget_open_window_measure(&size_pos, SDL_WINDOW_TOOLTIP, &self->context_menu.ctx.window, &self->context_menu.ctx.renderer, (widget_t)&buffer_context_menu_dispatch, self);
    }
    if (e->button.button == SDL_BUTTON_LEFT) {
      self->selection.state = SELECTION_DRAGGING_MOUSE;
      buffer_iter_screen_xy(self, &self->cursor.pos, e->button.x, e->button.y, true);
      self->selection.mark_1 = self->cursor.pos;
    }
    goto _view;
  }
  if (e->type == SDL_MOUSEBUTTONUP) {
    if (e->button.button == SDL_BUTTON_LEFT && self->selection.state == SELECTION_DRAGGING_MOUSE) {
      int mark1_offset = bs_offset(&self->selection.mark_1);
      int mark2_offset = bs_offset(&self->cursor.pos);
      self->selection.state = mark2_offset == mark1_offset ? SELECTION_INACTIVE : SELECTION_COMPLETE;
      self->selection.mark_2 = self->cursor.pos;
    }
    goto _view;
  }

  if (e->type == SDL_KEYDOWN) {
    if (e->key.keysym.scancode == SDL_SCANCODE_X && (e->key.keysym.mod & KMOD_CTRL)) {
      self->_prev_keysym = e->key.keysym;
      goto _noop;
    }

    if (e->key.keysym.scancode == SDL_SCANCODE_S && (e->key.keysym.mod & KMOD_CTRL)
        && self->_prev_keysym.scancode == SDL_SCANCODE_X && (self->_prev_keysym.mod & KMOD_CTRL)) {
      buff_string_iter_t iter;
      bs_begin(&iter, &self->contents);
      int buf_len = 1024 * 64;
      char buf[buf_len];
      remove(self->path);
      int fd = open(self->path, O_WRONLY|O_CREAT|O_TRUNC, 0644);
      for (;;) {
        int taken = bs_take_2(&iter, buf, buf_len);
        int written = 0;
        for(;;) {
          int w = write(fd, buf + written, taken - written);
          // TODO: handle write error
          if (w == -1) goto _view_command;
          written += w;
          if (written >= taken) break;
        }
        if (taken < buf_len) break;
      }
      close(fd);
      __auto_type path = self->path;
      __auto_type font_size = self->font.font_size;
      buffer_free(self);
      buffer_init(self, path, font_size);
      goto _view_command;
    }

    if (e->key.keysym.scancode == SDL_SCANCODE_LEFT
        || e->key.keysym.scancode == SDL_SCANCODE_B && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_left);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RIGHT
        || e->key.keysym.scancode == SDL_SCANCODE_F && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_right);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_DOWN && e->key.keysym.mod == 0
        || e->key.keysym.scancode == SDL_SCANCODE_N && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_down);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_UP && e->key.keysym.mod == 0
        || e->key.keysym.scancode == SDL_SCANCODE_P && (e->key.keysym.mod & KMOD_CTRL)
        ) {
      MODIFY_CURSOR(cursor_up);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_F && (e->key.keysym.mod & KMOD_ALT)) {
      MODIFY_CURSOR(cursor_forward_word);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_B && (e->key.keysym.mod & KMOD_ALT)) {
      MODIFY_CURSOR(cursor_backward_word);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_E && (e->key.keysym.mod & KMOD_CTRL)) {
      MODIFY_CURSOR(cursor_eol);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_A && (e->key.keysym.mod & KMOD_CTRL)) {
      MODIFY_CURSOR(cursor_bol);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_V && (e->key.keysym.mod & KMOD_CTRL)
        || e->key.keysym.scancode == SDL_SCANCODE_PAGEDOWN && (e->key.keysym.mod==0)
        ) {
      SDL_Rect viewport;
      SDL_RenderGetViewport(self->ctx.renderer, &viewport);
      scroll_page(&self->scroll, &self->cursor, &self->font, viewport.h, 1);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_D && (e->key.keysym.mod & KMOD_LGUI)) {
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_X && (e->key.keysym.mod & KMOD_ALT)) {
      goto _view_command;
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
      goto _view_command;
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
      goto _view_command;
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
      goto _view_command;
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
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_V && (e->key.keysym.mod & KMOD_ALT)
        || e->key.keysym.scancode == SDL_SCANCODE_PAGEUP && (e->key.keysym.mod==0)
        ) {
      SDL_Rect viewport;
      SDL_RenderGetViewport(self->ctx.renderer, &viewport);
      scroll_page(&self->scroll, &self->cursor, &self->font, viewport.h, -1);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(backward_paragraph);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(forward_paragraph);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_EQUALS  && (e->key.keysym.mod & KMOD_CTRL)) {
      TTF_CloseFont(self->font.font);
      self->font.font_size += 1;
      draw_init_font(&self->font, palette.monospace_font_path, self->font.font_size);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_MINUS && (e->key.keysym.mod & KMOD_CTRL)) {
      TTF_CloseFont(self->font.font);
      self->font.font_size -= 1;
      draw_init_font(&self->font, palette.monospace_font_path, self->font.font_size);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_PERIOD && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(cursor_end);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_COMMA && (e->key.keysym.mod & KMOD_ALT && e->key.keysym.mod & KMOD_SHIFT)) {
      MODIFY_CURSOR(cursor_begin);
      goto _view_command;
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
      goto _view_command;
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
      goto _view_command;
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

      self->_last_command = false;
      self->_prev_keysym = zero_keysym;
      goto _view;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_SLASH && (e->key.keysym.mod & KMOD_CTRL)) {
      self->contents = bs_insert_undo(self->contents, &self->cursor.pos, &self->scroll.pos, NULL);
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_SPACE && (e->key.keysym.mod & KMOD_CTRL)) {
      self->selection.state = SELECTION_COMPLETE;
      self->selection.mark_1 = self->cursor.pos;
      self->selection.mark_2 = self->cursor.pos;
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_G && (e->key.keysym.mod & KMOD_CTRL)) {
      self->selection.state = SELECTION_INACTIVE;
      self->selection.mark_1 = self->cursor.pos;
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_W && (e->key.keysym.mod & KMOD_CTRL)) {
      if (self->selection.state) {
        msg->tag = BUFFER_CUT;
        self->_last_command = true;
        self->_prev_keysym = zero_keysym;
        return;
      }
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_W && (e->key.keysym.mod & KMOD_ALT)) {
      if (self->selection.state) {
        buff_string_iter_t mark_1 = self->selection.mark_1;
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
        self->selection.state = SELECTION_INACTIVE;
      }
      goto _view_command;
    }
    if (e->key.keysym.scancode == SDL_SCANCODE_F10) {
      self->show_lines = self->show_lines ? false : true;
      buffer_get_geometry(self, &self->geometry);
      goto _view_command;
    }
    self->_last_command = false;
    goto _noop;
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
      goto _view;
    }
  }

  if (e->type == SDL_WINDOWEVENT) {
    if (e->window.event == SDL_WINDOWEVENT_ENTER) {
      SDL_SetCursor(self->ibeam_cursor);
      goto _noop;
    }
    if (e->window.event == SDL_WINDOWEVENT_LEAVE) {
      SDL_SetCursor(SDL_GetDefaultCursor());
      goto _noop;
    }
    if (e->window.event == SDL_WINDOWEVENT_EXPOSED) {
      buffer_get_geometry(self, &self->geometry);
      goto _view;
    }
    if (e->window.event == SDL_WINDOWEVENT_RESIZED) {
      buffer_get_geometry(self, &self->geometry);
      goto _view;
    }
  }

  _noop_command:
  self->_last_command = true;
  self->_prev_keysym = zero_keysym;
  return;
  _view_command:
  self->_last_command = true;
  self->_prev_keysym = zero_keysym;
  yield(&msg_view);
  return;
  _noop:
  return;
  _view:
  yield(&msg_view);
  return;
}

static void cursor_modified (
  buffer_t *self,
  cursor_t *prev
) {
  SDL_Rect viewport;
  SDL_RenderGetViewport(self->ctx.renderer, &viewport);
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

static buffer_context_menu_item_t items[] = {
  {.title="Cut", .icon = 0xf0c4, .action = BUFFER_CUT},
  {.title="Copy", .icon = 0xf328, .action = BUFFER_COPY},
  {.title="Paste", .icon = 0xf15c, .action = BUFFER_PASTE},
};

static void
buffer_context_menu_init(menulist_t *self) {
  menulist_init(self, &items->menulist_item, sizeof(items)/sizeof(menulist_item_t), sizeof(menulist_item_t));
}

void
buffer_context_menu_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  if (msg->tag < MSG_USER) {
    buffer_msg_t msg1 = {.tag = BUFFER_CONTEXT_MENU, .context_menu = *(menulist_msg_t *)msg};
    buffer_dispatch(self, &msg1, yield);
  } else {
    buffer_dispatch(self, (void *)msg, yield);
  }
}
