#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "input.h"
#include "draw.h"
#include "main.h"
#include "widget.h"

void input_view_lines(input_t *self, SDL_Rect *viewport);
static SDL_Keysym zero_keysym = {0};

void input_init(input_t *self, buff_string_t *content) {
  draw_init_font(&self->font, palette.default_font_path, 16);
  self->contents = content;

  self->selection.state = SELECTION_INACTIVE;
  bs_index(&self->contents, &self->selection.mark_1, 0);
  bs_index(&self->contents, &self->selection.mark_2, 0);

  draw_init_context(&self->ctx, &self->font);
  self->scroll.line = 0;
  bs_begin(&self->scroll.pos, &self->contents);
  bs_begin(&self->cursor.pos, &self->contents);
  self->cursor.x0 = 0;
  self->font.font_size = 16;
  self->lines = NULL;
  self->lines_len = 0;
  self->_last_command = false;
  self->_prev_keysym = zero_keysym;
  self->context_menu.ctx = self->ctx;
  self->context_menu.ctx.window = NULL;
  self->ibeam_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
}

void input_free(input_t *self) {
  TTF_CloseFont(self->font.font);
  SDL_FreeCursor(self->ibeam_cursor);
}

void input_view(input_t *self) {
}

bool input_iter_screen_xy(input_t *self, buff_string_iter_t *iter, int screen_x, int screen_y, bool x_adjust) {
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

  int current_x = 0;
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

void input_dispatch(input_t *self, input_msg_t *msg, yield_t yield) {
  void yield_context_menu(void *msg) {
    input_msg_t input_msg = {.tag = INPUT_CONTEXT_MENU, .context_menu = *(menulist_msg_t *)msg};
    yield(&input_msg);
  }

  switch (msg->tag) {
    case MSG_SDL_EVENT: {
      // input_dispatch_sdl(self, msg, yield, &yield_context_menu);
      return;
    }
    case MSG_FREE: {
      input_free(self);
      return;
    }
    case MSG_VIEW: {
      input_view(self);
      return;
    }
    case MSG_MEASURE: {
      msg->widget.measure.result->x = INT_MAX;
      msg->widget.measure.result->y = INT_MAX;
      return;
    }
    case INPUT_CUT: {
      buff_string_iter_t mark_1 = self->selection.mark_1;
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
      self->selection.state = SELECTION_INACTIVE;
      yield(&msg_view);
      return;
    }
    case INPUT_CONTEXT_MENU: {
      if (msg->context_menu.tag == MENULIST_ITEM_CLICKED) {
        input_msg_t next_msg = {.tag = msg->context_menu.item_clicked->action};
        yield(&next_msg);
        widget_close_window(self->context_menu.ctx.window);
        self->context_menu.ctx.window = NULL;
        return;
      }
      if (msg->context_menu.tag == MENULIST_DESTROY) {
        widget_close_window(self->context_menu.ctx.window);
        self->context_menu.ctx.window = NULL;
        return;
      }
      menulist_dispatch(&self->context_menu, &msg->context_menu, &yield_context_menu);
      if (msg->context_menu.tag == MSG_VIEW) {
        SDL_RenderPresent(self->context_menu.ctx.renderer);
      }
      return;
    }
    default: {
      return;
    }
  }
}

SDL_Point selection_get_range(selection_t *selection, cursor_t *cursor) {
  SDL_Point result;
  result.x
    = selection->state != SELECTION_INACTIVE ? bs_offset(&selection->mark_1)
    : -1;
  result.y
    = selection->state == SELECTION_COMPLETE ? bs_offset(&selection->mark_2)
    : selection->state == SELECTION_DRAGGING_KEYBOARD ? bs_offset(&cursor->pos)
    : selection->state == SELECTION_DRAGGING_MOUSE ? bs_offset(&cursor->pos)
    : -1;
  if (result.x > result.y) {
    __auto_type temp = result.x;
    result.x = result.y;
    result.y = temp;
  }
  return result;
}

int input_unittest() {
}
