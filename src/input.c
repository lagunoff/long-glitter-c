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
void input_update_lines(input_t *self, SDL_Rect *viewport);

static const SDL_Keysym zero_keysym = {0};
static const int CURSOR_LINE_WIDTH = 2;

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
  __auto_type ctx = &self->ctx;
  __auto_type inside_selection = false;
  __auto_type iter = self->scroll.pos;
  __auto_type cursor_offset = bs_offset(&self->cursor.pos);
  __auto_type scroll_offset = bs_offset(&self->scroll.pos);
  __auto_type sel_range = selection_get_range(&self->selection, &self->cursor);

  char temp[1024 * 16]; // Maximum line length — 16kb

  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  input_update_lines(self, &viewport);
  draw_set_color(ctx, ctx->background);

  int y = 0; // y coordinate of the top-left corner of the text
  int i = 0; // Index of the line
  for(;;i++) {
    draw_set_color(ctx, ctx->palette->primary_text);
    __auto_type begin_offset = bs_offset(&iter);
    __auto_type eof = bs_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    __auto_type end_offset = bs_offset(&iter);
    __auto_type line_len = end_offset - begin_offset;
    self->lines[i] = begin_offset;

    if (cursor_offset >= begin_offset && cursor_offset <= end_offset) {
      draw_set_color(ctx, ctx->palette->current_line_bg);
      draw_box(ctx, 0, y, viewport.w, ctx->font->X_height);
      draw_set_color(ctx, ctx->palette->primary_text);
    }

    // Whole selection inside the current line
    if (sel_range.y >= begin_offset && sel_range.y <= end_offset
      && sel_range.x >= begin_offset && sel_range.x <= end_offset) {
      int len = end_offset - begin_offset;
      int len1 = sel_range.x - begin_offset;
      int len2 = sel_range.y - begin_offset - len1;
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

    // Selection starts inside the current line and continues further
    if (sel_range.x >= begin_offset && sel_range.x <= end_offset) {
      int len1 = sel_range.x - begin_offset;
      int len2 = end_offset - sel_range.x;
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

    // Selection ends inside the current line
    if (sel_range.y >= begin_offset && sel_range.y <= end_offset) {
      int len1 = sel_range.y - begin_offset;
      int len2 = end_offset - sel_range.y;
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
      int x = CURSOR_LINE_WIDTH / 2;
      draw_set_color(ctx, ctx->palette->primary_text);
      draw_text(ctx, x, y, temp);
      /* c_mode_highlight(&c_mode, temp, line_len, lambda(void _(char *start, int len, c_mode_state_t state){ */
      /*   char t[len + 1]; */
      /*   SDL_Point temp_size; */
      /*   strncpy(t, start, len); */
      /*   t[len] = '\0'; */
      /*   draw_set_color(ctx, c_mode_choose_color(ctx, state)); */
      /*   draw_text(ctx, x, y, t); */
      /*   draw_set_color(ctx, ctx->palette->primary_text); */
      /*   draw_measure_text(ctx, t, &temp_size); */
      /*   x += temp_size.x; */
      /* })); */
    }
  draw_cursor:
    if (cursor_offset >= begin_offset && cursor_offset <= end_offset) {
      int cur_x_offset = cursor_offset - begin_offset;
      int cursor_char = temp[cur_x_offset];
      temp[cur_x_offset]='\0';
      SDL_Point te;
      draw_measure_text(ctx, temp, &te);
      draw_box(ctx, te.x, y - 2, CURSOR_LINE_WIDTH, ctx->font->X_height + 2);

      /* if (cursor_char != '\0' && cursor_char != '\n') { */
      /*   temp[0]=cursor_char; */
      /*   temp[1]='\0'; */
      /*   draw_set_color_rgba(ctx, 1, 1, 1, 1); */
      /*   draw_text(ctx, te.x, y, temp); */
      /* } */
    }
    y += ctx->font->X_height;
    if (y + ctx->font->X_height >= viewport.h) break;
    // Skip newline symbol
    bs_move(&iter, 1);
    if (eof) break;
  }

  self->lines[i++] = bs_offset(&iter);
  for (;i < self->lines_len; i++) self->lines[i] = -1;
}

void input_update_lines(input_t *self, SDL_Rect *viewport) {
  __auto_type ctx = &self->ctx;
  int lines_len = div(viewport->h + ctx->font->X_height, ctx->font->X_height).quot + 1;

  if (!self->lines || (lines_len != self->lines_len)) {
    self->lines_len = lines_len;
    self->lines = realloc(self->lines, self->lines_len * sizeof(int));
    if (self->lines_len == 0) self->lines = NULL;
  }
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
