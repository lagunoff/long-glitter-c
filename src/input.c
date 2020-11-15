#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "input.h"
#include "draw.h"
#include "utils.h"
#include "widget.h"

void input_view_lines(input_t *self);
void input_update_lines(input_t *self);
void input_line_selection_range(point_t *line_sel_range, point_t *sel_range, int begin_offset, int end_offset);

static const int CURSOR_LINE_WIDTH = 2;

void input_init(input_t *self, draw_context_init_t *ctx, buff_string_t *content) {
  self->contents = content;

  self->selection.state = SELECTION_INACTIVE;
  bs_index(&self->contents, &self->selection.mark_1, 0);
  bs_index(&self->contents, &self->selection.mark_2, 0);

  draw_init_context(&self->ctx, ctx);
  self->scroll.line = 0;
  bs_begin(&self->scroll.pos, &self->contents);
  bs_begin(&self->cursor.pos, &self->contents);
  self->cursor.x0 = 0;
  self->lines = NULL;
  self->lines_len = 0;
  self->syntax_hl = &noop_highlighter;
  self->context_menu.ctx = self->ctx;
  //  self->ibeam_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
}

void input_free(input_t *self) {
}

void input_set_style(draw_context_t *ctx, text_style_t *style) {
  draw_set_color(ctx, draw_get_color_from_style(ctx, style->syntax));
}

void input_view(input_t *self) {
  auto void with_styles(highlighter_t hl);
  __auto_type ctx = &self->ctx;
  __auto_type iter = self->scroll.pos;
  __auto_type cursor_offset = bs_offset(&self->cursor.pos);
  __auto_type scroll_offset = bs_offset(&self->scroll.pos);
  __auto_type sel_range = selection_get_range(&self->selection, &self->cursor);
  __auto_type max_y = ctx->clip.y + ctx->clip.h;

  char temp[1024 * 16]; // Maximum line length — 16kb
  int y = 0; // y coordinate of the top-left corner of the text
  int i = 0; // Index of the line
  point_t line_sel_range;
  text_style_t normal = {.syntax = SYNTAX_NORMAL, .selected = false};
  highlighter_args_t hl_args = {.ctx = ctx, .normal = &normal, .input = temp, .len = 0};

  input_update_lines(self);
  draw_set_color(ctx, ctx->background);
  draw_rect(ctx, ctx->clip);

  for(;;i++) {
    draw_set_color(ctx, ctx->palette->primary_text);
    __auto_type begin_offset = bs_offset(&iter);
    __auto_type eof = bs_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    __auto_type end_offset = bs_offset(&iter);
    __auto_type line_len = end_offset - begin_offset;
    hl_args.len = line_len;
    input_line_selection_range(&line_sel_range, &sel_range, begin_offset, end_offset);
    self->lines[i] = begin_offset;

    // Highlight the current line
    if (cursor_offset >= begin_offset && cursor_offset <= end_offset) {
      draw_set_color(ctx, ctx->palette->current_line_bg);
      draw_box(ctx, 0, y, ctx->clip.w, ctx->font->height);
      draw_set_color(ctx, ctx->palette->primary_text);
    }

    // Draw the line
    with_styles(lambda(void _(point_t r, text_style_t *s) {
      input_set_style(ctx, s);
      if (r.y - r.x > 0) draw_text(ctx, 0, y + ctx->font->ascent, temp + r.x, r.y - r.x);
    }));

    // Draw cursor
    if (cursor_offset >= begin_offset && cursor_offset <= end_offset) {
      int cur_x_offset = cursor_offset - begin_offset;
      XGlyphInfo extents;
      draw_measure_text(ctx, temp, cur_x_offset, &extents);
      draw_box(ctx, extents.x, y - 2, CURSOR_LINE_WIDTH, ctx->font->height + 2);
    }
    y += ctx->font->height;
    if (y + ctx->font->height >= max_y) break;
    // Skip newline symbol
    bs_move(&iter, 1);
    if (eof) break;
  }

  self->lines[i++] = bs_offset(&iter);
  for (;i < self->lines_len; i++) self->lines[i] = -1;

  void selection_on(text_style_t *s) {s->selected = true;}
  void selection_off(text_style_t *s) {s->selected = false;}
  void with_styles_1(highlighter_t hl) {self->syntax_hl->highlight(self->syntax_hl_inst, &hl_args, hl);}
  void with_styles(highlighter_t hl) {input_highlight_range(&selection_on, &selection_off, line_sel_range, &with_styles_1, hl);}
}

void input_update_lines(input_t *self) {
  __auto_type ctx = &self->ctx;
  int lines_len = div(ctx->clip.h + ctx->font->height, ctx->font->height).quot + 1;

  if (!self->lines || (lines_len != self->lines_len)) {
    self->lines_len = lines_len;
    self->lines = realloc(self->lines, self->lines_len * sizeof(int));
    if (self->lines_len == 0) self->lines = NULL;
  }
}

bool input_iter_screen_xy(input_t *self, buff_string_iter_t *iter, int screen_x, int screen_y, bool x_adjust) {
  if (self->lines_len < 0) return false;
  __auto_type ctx = &self->ctx;
  int x = screen_x - ctx->clip.x;
  int y = screen_y - ctx->clip.y;
  int line_y = div(y, ctx->font->height).quot;
  int line_offset = self->lines[line_y];
  if (line_offset < 0) return false;
  int minx, maxx, miny, maxy, advance;

  int current_x = 0;
  bs_index(&self->contents, iter, line_offset);
  bs_find(iter, lambda(bool _(char c){
    if (c == '\n') return true;
    // TTF_GlyphMetrics(ctx->font->font, c, &minx, &maxx, &miny, &maxy, &advance);
    current_x += advance;
    // TODO: better end of line detection
    if (current_x - 18 >= x) return true;
    return false;
  }));
  return true;
}

void input_dispatch(input_t *self, input_msg_t *msg, yield_t yield) {
  auto void yield_context_menu(void *msg);

  switch (msg->tag) {
  case MSG_FREE: {
    input_free(self);
    return;
  }
  case MSG_VIEW: {
    input_view(self);
    return;
  }
  case MSG_MEASURE: {
    msg->widget.measure->x = INT_MAX;
    msg->widget.measure->y = INT_MAX;
    return;
  }
  case INPUT_CUT: {
    __auto_type mark_1 = self->selection.mark_1;
    __auto_type mark_2 = self->cursor.pos;
    if (mark_1.global_index > mark_2.global_index) {
      __auto_type tmp = mark_1;
      mark_1 = mark_2;
      mark_2 = tmp;
    }
    __auto_type len = mark_2.global_index - mark_1.global_index;
    __auto_type mark_1_temp = mark_1;
    char temp[len + 1];
    bs_take(&mark_1_temp, temp, len);
    // SDL_SetClipboardText(temp);
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
      self->context_menu.ctx.window = 0;
      return;
    }
    if (msg->context_menu.tag == MENULIST_DESTROY) {
      widget_close_window(self->context_menu.ctx.window);
      self->context_menu.ctx.window = 0;
      return;
    }
    menulist_dispatch(&self->context_menu, &msg->context_menu, &yield_context_menu);
    if (msg->context_menu.tag == MSG_VIEW) {
      // SDL_RenderPresent(self->context_menu.ctx.renderer);
    }
    return;
  }}

  void yield_context_menu(void *msg) {
    input_msg_t input_msg = {.tag = INPUT_CONTEXT_MENU, .context_menu = *(menulist_msg_t *)msg};
    yield(&input_msg);
  }
}

point_t selection_get_range(selection_t *selection, cursor_t *cursor) {
  point_t result;
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

void input_line_selection_range(point_t *line_sel_range, point_t *sel_range, int begin_offset, int end_offset) {
  __auto_type line_len = end_offset - begin_offset;
  line_sel_range->x = -1;
  line_sel_range->y = -1;
  if (sel_range->x >= begin_offset && sel_range->x < end_offset) {
    line_sel_range->x = begin_offset - sel_range->x;
    line_sel_range->y = MIN(begin_offset - sel_range->y, line_len);
  }
  if (sel_range->y >= begin_offset && sel_range->y < end_offset) {
    line_sel_range->y = begin_offset - sel_range->y;
    line_sel_range->y = MAX(begin_offset - sel_range->x, 0);
  }
}

void noop() {}
void noop_highlight(void *self, highlighter_args_t *args, highlighter_t hl) {
  point_t range = {0, args->len};
  hl(range, args->normal);
}

static __attribute__((constructor)) void __init__() {
  noop_highlighter.reset = &noop;
  noop_highlighter.forward = &noop;
  noop_highlighter.highlight = &noop_highlight;
}
