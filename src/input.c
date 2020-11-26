#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include "graphics.h"
#include "input.h"
#include "utils.h"
#include "widget.h"

//! Moving cursor can cause it to leave the visible area this macros
//! restores proper scroll position after cursor movement
#define modify_cursor(f) {                                   \
  cursor_t old_cursor = self->cursor;                        \
  f(&self->cursor);                                          \
  cursor_modified(self, &old_cursor);                        \
}

//! Number of lines to scroll when cursor is moved near the boundaries
#define SCROLL_JUMP 8
//! How much to scroll by mouse wheel
#define SCROLL_FACTOR 4

void input_view_lines(input_t *self);
void input_update_lines(input_t *self);
void input_line_selection_range(point_t *line_sel_range, point_t *sel_range, int begin_offset, int end_offset);
static void cursor_modified(input_t *self,cursor_t *prev);

static const int CURSOR_LINE_WIDTH = 2;
XEvent respond;

void input_init(input_t *self, widget_context_init_t *ctx, buff_string_t *content, syntax_highlighter_t *hl) {
  self->contents = content;

  self->selection.state = Selection_Inactive;
  bs_index(&self->contents, &self->selection.mark_1, 0);
  bs_index(&self->contents, &self->selection.mark_2, 0);

  gx_init_context(&self->ctx, ctx);
  self->scroll.line = 0;
  bs_begin(&self->scroll.pos, &self->contents);
  bs_begin(&self->cursor.pos, &self->contents);
  self->cursor.x0 = 0;
  self->lines = NULL;
  self->lines_len = 0;
  self->syntax_hl = hl;
  hl->reset(&self->syntax_hl_inst);
  self->context_menu.ctx = self->ctx;
  self->x_selection = NULL;
}

void input_free(input_t *self) {
  if (self->x_selection) free(self->x_selection);
}

void input_dispatch(input_t *self, input_msg_t *msg, yield_t yield) {
  auto void yield_context_menu(void *msg);
  auto void do_insert_fixup(iter_fixup_t fix_cursor, iter_fixup_t fix_scroll);
  __auto_type ctx = &self->ctx;

  switch (msg->tag) {
  case Expose: {
    return input_view(self);
  }
  case EnterNotify: {
    XDefineCursor(ctx->display, ctx->window, self->ctx.palette->xterm);
    return;
  }
  case LeaveNotify: {
    XDefineCursor(ctx->display, ctx->window, None);
    return;
  }
  case MotionNotify: {
    __auto_type motion = &msg->widget.x_event->xmotion;
    if (motion->state & Button1MotionMask) {
      if (self->selection.state != Selection_DraggingMouse) {
        self->selection.state = Selection_DraggingMouse;
        self->selection.mark_1 = self->cursor.pos;
      }
      input_iter_screen_xy(self, &self->cursor.pos, motion->x, motion->y, true);
      return yield(&msg_view);
    } else {
      if (self->selection.state ==Selection_DraggingMouse) {
        self->selection.state = Selection_Complete;
        self->selection.mark_2 = self->cursor.pos;
      }
    }
    return;
  }
  case ButtonPress: {
    __auto_type button = &msg->widget.x_event->xbutton;
    switch (button->button) {
    case Button1: { // Left click
      input_iter_screen_xy(self, &self->cursor.pos, button->x, button->y, true);
      return yield(&msg_view);
    }
    case Button4: { // Wheel up
      scroll_lines(&self->scroll, -1 * SCROLL_FACTOR);
      return yield(&msg_view);
    }
    case Button5: { // Wheel down
      scroll_lines(&self->scroll, 1 * SCROLL_FACTOR);
      return yield(&msg_view);
    }}
    return;
  }
  case Widget_Free: {
    return input_free(self);
  }
  case Widget_Measure: {
    msg->widget.measure.x = INT_MAX;
    msg->widget.measure.y = INT_MAX;
    return;
  }
  case SelectionNotify: {
    __auto_type pty = XInternAtom(ctx->display, "XCLIP_OUT", false);
    unsigned long pty_size, pty_items;
    int pty_format;
    unsigned char *clipboard = NULL;
    Atom pty_type;
    XGetWindowProperty(
      ctx->display,
      ctx->window,
      pty,0,0,False,
      AnyPropertyType,
      &pty_type, &pty_format, &pty_items, &pty_size, &clipboard
    );
    XFree(clipboard);
    XGetWindowProperty(
      ctx->display,
      ctx->window,
      pty, 0, (long)pty_size, False,
      AnyPropertyType,
      &pty_type, &pty_format, &pty_items, &pty_size, &clipboard
    );
    XDeleteProperty(ctx->display, ctx->window, pty);
    if (clipboard) {
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        clipboard, 0, BuffString_Left,
        &do_insert_fixup
      );
      XFree(clipboard);
      return yield(&msg_view);
    }
    return;
  }
  case SelectionRequest: {
    // Copy self->x_selection into the clipboard
    __auto_type targets = XInternAtom(ctx->display, "TARGETS", false);
    __auto_type req = &msg->widget.x_event->xselectionrequest;

    if (req->target == targets) {
      Atom types[2] = { targets, XA_UTF8_STRING(ctx->display) };
      XChangeProperty(
        ctx->display,
        req->requestor,
        req->property,
        XA_ATOM,
        32, PropModeReplace, (unsigned char *) types,
        (int) (sizeof(types) / sizeof(Atom))
      );
    } else {
      XChangeProperty(
        ctx->display,
        req->requestor,
        req->property,
        req->target,
        8, PropModeReplace,
        self->x_selection,strlen(self->x_selection) + 1
      );
    }

    respond.type = SelectionNotify;
    respond.xselection.property = req->property;
    respond.xselection.display = req->display;
    respond.xselection.requestor = req->requestor;
    respond.xselection.selection = req->selection;
    respond.xselection.time = req->time;
    respond.xselection.target = req->target;

    XSendEvent(ctx->display, req->requestor, 0, 0, &respond);
    XFlush(ctx->display);
    return;
  }
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event->xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    __auto_type is_ctrl = xkey->state & ControlMask;
    __auto_type is_alt = xkey->state & Mod1Mask;
    __auto_type is_altshift = (xkey->state & Mod1Mask) && (xkey->state & ShiftMask);

    if (keysym == XK_Up || (keysym == XK_p && is_ctrl)) {
      modify_cursor(cursor_up);
      return yield(&msg_view);
    } else if (keysym == XK_Down || (keysym == XK_n && is_ctrl)) {
      modify_cursor(cursor_down);
      return yield(&msg_view);
    } else if (keysym == XK_Left || (keysym == XK_b && is_ctrl)) {
      modify_cursor(cursor_left);
      return yield(&msg_view);
    } else if (keysym == XK_Right || (keysym == XK_f && is_ctrl)) {
      modify_cursor(cursor_right);
      return yield(&msg_view);
    } else if (keysym == XK_f && is_alt) {
      modify_cursor(cursor_forward_word);
      return yield(&msg_view);
    } else if (keysym == XK_b && is_alt) {
      modify_cursor(cursor_backward_word);
      return yield(&msg_view);
    } else if (keysym == XK_e && is_ctrl) {
      modify_cursor(cursor_eol);
      return yield(&msg_view);
    } else if (keysym == XK_a && is_ctrl) {
      modify_cursor(cursor_bol);
      return yield(&msg_view);
    } else if (keysym == XK_v && is_ctrl || keysym == XK_Page_Down) {
      scroll_page(&self->ctx, &self->scroll, &self->cursor, 1);
      return yield(&msg_view);
    } else if (keysym == XK_v && is_alt || keysym == XK_Page_Up) {
      scroll_page(&self->ctx, &self->scroll, &self->cursor, -1);
      return yield(&msg_view);
    } else if (keysym == XK_bracketleft && is_altshift) {
      modify_cursor(backward_paragraph);
      return yield(&msg_view);
    } else if (keysym == XK_bracketright && is_altshift) {
      modify_cursor(forward_paragraph);
      return yield(&msg_view);
    } else if (keysym == XK_k && is_ctrl) {
      __auto_type iter = self->cursor.pos;
      bs_find(&iter, lambda(bool _(char c) { return c == '\n'; }));
      __auto_type curr_offset = bs_offset(&self->cursor.pos);
      __auto_type iter_offset = bs_offset(&iter);
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", MAX(iter_offset - curr_offset, 1),
        BuffString_Right,
        &do_insert_fixup
      );
      return yield(&msg_view);
    } else if (keysym == XK_BackSpace && is_alt) {
      __auto_type iter = self->cursor.pos;
      bs_backward_word(&iter);
      __auto_type curr_offset = bs_offset(&self->cursor.pos);
      __auto_type iter_offset = bs_offset(&iter);
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", MAX(curr_offset - iter_offset, 1),
        BuffString_Left,
        &do_insert_fixup
      );
      return yield(&msg_view);
    } else if (keysym == XK_d && is_alt) {
      __auto_type iter = self->cursor.pos;
      bs_forward_word(&iter);
      __auto_type curr_offset = bs_offset(&self->cursor.pos);
      __auto_type iter_offset = bs_offset(&iter);
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", MAX(iter_offset - curr_offset, 1),
        BuffString_Right,
        &do_insert_fixup
      );
      return yield(&msg_view);
    } else if (keysym == XK_comma && is_altshift) {
      modify_cursor(cursor_begin);
      return yield(&msg_view);
    } else if (keysym == XK_period && is_altshift) {
      modify_cursor(cursor_end);
      return yield(&msg_view);
    } else if ((keysym == XK_Delete) || (keysym == XK_d && is_ctrl)) {
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", 1,
        BuffString_Right,
        &do_insert_fixup
      );
      return yield(&msg_view);
    } else if (keysym == XK_BackSpace) {
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "", 1,
        BuffString_Left,
        &do_insert_fixup
      );
      return yield(&msg_view);
    } else if (keysym == XK_Return) {
      self->contents = bs_insert(
        self->contents,
        self->cursor.pos.global_index,
        "\n", 0,
        BuffString_Left,
        &do_insert_fixup
      );
      return yield(&msg_view);
    } else if (keysym == XK_slash && is_ctrl) {
      self->contents = bs_insert_undo(self->contents, &self->cursor.pos, &self->scroll.pos, NULL);
      return yield(&msg_view);
    } else if (keysym == XK_space && is_ctrl) {
      self->selection.state = Selection_DraggingKeyboard;
      self->selection.mark_1 = self->cursor.pos;
      self->selection.mark_2 = self->cursor.pos;
      return yield(&msg_view);
    } else if (keysym == XK_g && is_ctrl) {
      self->selection.state = Selection_Inactive;
      self->selection.mark_1 = self->cursor.pos;
      return yield(&msg_view);
    } else if (keysym == XK_w && is_ctrl) {
      input_msg_t next_msg = {.tag = Input_Cut};
      return yield(&next_msg);
    } else if (keysym == XK_w && is_alt) {
      input_msg_t next_msg = {.tag = Input_Copy};
      return yield(&next_msg);
    } else if (keysym == XK_y && is_ctrl) {
      input_msg_t next_msg = {.tag = Input_Paste};
      return yield(&next_msg);
    } else {
      char buffer[32];
      KeySym ignore;
      Status return_status;
      Xutf8LookupString(self->ctx.xic, xkey, buffer, 32, &ignore, &return_status);
      if (strlen(buffer)) {
        self->contents = bs_insert(
          self->contents,
          self->cursor.pos.global_index,
          buffer, 0,
          BuffString_Left,
          &do_insert_fixup
        );
        return yield(&msg_view);
      }
      return;
    }
    return;
  }
  case Input_Cut: {
    if (self->selection.state == Selection_Complete) {
      __auto_type mark_1 = self->selection.mark_1;
      __auto_type mark_2 = self->cursor.pos;
      if (mark_1.global_index > mark_2.global_index) swap(mark_1, mark_2);
      __auto_type len = mark_2.global_index - mark_1.global_index;
      __auto_type mark_1_temp = mark_1;
      self->x_selection = realloc(self->x_selection, len + 1);
      bs_take(&mark_1, self->x_selection, len);
      XSetSelectionOwner(ctx->display, XA_PRIMARY, ctx->window, CurrentTime);
      self->contents = bs_insert(
        self->contents,
        mark_1.global_index,
        "", len,
        BuffString_Right,
        &do_insert_fixup
      );
      self->selection.state = Selection_Inactive;
      return yield(&msg_view);
    }
    return;
  }
  case Input_Copy: {
    if (self->selection.state == Selection_Complete) {
      __auto_type mark_1 = self->selection.mark_1;
      __auto_type mark_2 = self->cursor.pos;
      if (mark_1.global_index > mark_2.global_index) swap(mark_1, mark_2);
      __auto_type len = mark_2.global_index - mark_1.global_index;
      self->x_selection = realloc(self->x_selection, len + 1);
      bs_take(&mark_1, self->x_selection, len);
      XSetSelectionOwner(ctx->display, XA_PRIMARY, ctx->window, CurrentTime);
      self->selection.state = Selection_Inactive;
      return yield(&msg_view);
    }
    return;
  }
  case Input_Paste: {
    __auto_type pty = XInternAtom(ctx->display, "XCLIP_OUT", false);
    XConvertSelection(ctx->display, XA_PRIMARY, XA_UTF8_STRING(ctx->display), pty, ctx->window, CurrentTime);
    return;
  }
  case Input_ContextMenu: {
    if (msg->context_menu.tag == Menulist_ItemClicked) {
      input_msg_t next_msg = {.tag = msg->context_menu.item_clicked->action};
      yield(&next_msg);
      widget_close_window(self->context_menu.ctx.window);
      self->context_menu.ctx.window = 0;
      return;
    }
    if (msg->context_menu.tag == Menulist_Destroy) {
      widget_close_window(self->context_menu.ctx.window);
      self->context_menu.ctx.window = 0;
      return;
    }
    menulist_dispatch(&self->context_menu, &msg->context_menu, &yield_context_menu);
    if (msg->context_menu.tag == Expose) {
      // SDL_RenderPresent(self->context_menu.ctx.renderer);
    }
    return;
  }}

  void yield_context_menu(void *msg) {
    input_msg_t input_msg = {.tag = Input_ContextMenu, .context_menu = *(menulist_msg_t *)msg};
    yield(&input_msg);
  }
  void do_insert_fixup(iter_fixup_t fix_cursor, iter_fixup_t fix_scroll) {
    fix_cursor(&self->cursor.pos);
    fix_scroll(&self->scroll.pos);
  }
}

void input_set_style(widget_context_t *ctx, text_style_t *style) {
  // Uncomment if you want text inside selection to be white
  if (0 && style->selected) {
    gx_set_color_rgba(ctx, 1,1,1,1);
  } else {
    gx_set_color(ctx, gx_get_color_from_style(ctx, style->syntax));
  }
}

void input_view(input_t *self) {
  auto void with_styles(highlighter_t hl);
  __auto_type ctx = &self->ctx;
  __auto_type iter = self->scroll.pos;
  __auto_type cursor_offset = bs_offset(&self->cursor.pos);
  __auto_type scroll_offset = bs_offset(&self->scroll.pos);
  __auto_type sel_range = selection_get_range(&self->selection, &self->cursor);
  __auto_type max_y = ctx->clip.y + ctx->clip.h;
  char syntax_hl_inst[16];
  strncpy(syntax_hl_inst, self->syntax_hl_inst, sizeof(syntax_hl_inst));

  char temp[1024 * 16]; // Maximum line length — 16kb
  int y = ctx->clip.y; // y coordinate of the top-left corner of the text
  int i = 0; // Index of the line
  point_t line_sel_range;
  text_style_t normal = {.syntax = Syntax_Normal, .selected = false};
  highlighter_args_t hl_args = {.ctx = ctx, .normal = &normal, .input = temp, .len = strlen(temp)};

  input_update_lines(self);
  gx_set_font(&self->ctx, self->ctx.font);
  gx_set_color(&self->ctx, self->ctx.background);
  gx_rect(&self->ctx, self->ctx.clip);

  for(;;i++) {
    gx_set_color(ctx, ctx->palette->primary_text);
    __auto_type begin_offset = bs_offset(&iter);
    __auto_type eof = bs_takewhile(&iter, temp, lambda(bool _(char c) { return c != '\n'; }));
    __auto_type end_offset = bs_offset(&iter);
    __auto_type line_len = end_offset - begin_offset;
    hl_args.len = line_len;
    input_line_selection_range(&line_sel_range, &sel_range, begin_offset, end_offset);
    self->lines[i] = begin_offset;

    // Highlight the current line
    if (cursor_offset >= begin_offset && cursor_offset <= end_offset) {
      gx_set_color(ctx, ctx->palette->current_line_bg);
      gx_box(ctx, ctx->clip.x, y, ctx->clip.w, ctx->font->extents.height);
      gx_set_color(ctx, ctx->palette->primary_text);
    }

    // Draw the line
    // cairo_set_source_rgb(ctx->cairo, 0, 0, 0);
    /* gx_set_color(ctx, ctx->palette->primary_text); */
    /* gx_text(ctx, ctx->clip.x, y + ctx->font->extents.ascent, temp, 0); */
    int x = ctx->clip.x;
    with_styles(lambda(void _(point_t r, text_style_t *s) {
      __auto_type len = r.y - r.x;
      if (len > 0) {
        cairo_text_extents_t extents;
        char tmp[len + 1];
        strncpy(tmp, temp + r.x, len);
        tmp[len] = '\0';
        gx_measure_text(ctx, tmp, &extents);
        if (s->selected) {
          gx_set_color(ctx, ctx->palette->selection_bg);
          gx_box(ctx, x, y, extents.x_advance, ctx->font->extents.height);
        }
        input_set_style(ctx, s);
        gx_text(ctx, x, y + ctx->font->extents.ascent, tmp);
        x += extents.x_advance;
      }
    }));

    // Draw cursor
    if (cursor_offset >= begin_offset && cursor_offset <= end_offset) {
      int cur_x_offset = cursor_offset - begin_offset;
      temp[cur_x_offset] = '\0';
      cairo_text_extents_t extents;
      gx_measure_text(ctx, temp, &extents);
      gx_box(ctx, ctx->clip.x + extents.x_advance, y - 2, CURSOR_LINE_WIDTH, ctx->font->extents.height + 2);
    }
    y += ctx->font->extents.height;
    if (y + ctx->font->extents.height >= max_y) break;
    // Skip newline symbol
    bs_move(&iter, 1);
    if (eof) break;
  }

  self->lines[i++] = bs_offset(&iter);
  for (;i < self->lines_len; i++) self->lines[i] = -1;

  void selection_on(text_style_t *s) {s->selected = true;}
  void selection_off(text_style_t *s) {s->selected = false;}
  void with_styles_1(highlighter_t hl) {self->syntax_hl->highlight(syntax_hl_inst, &hl_args, hl);}
  void with_styles(highlighter_t hl) {input_highlight_range(&selection_on, &selection_off, line_sel_range, &with_styles_1, hl);}
}

void input_update_lines(input_t *self) {
  __auto_type ctx = &self->ctx;
  int lines_len = div(ctx->clip.h + ctx->font->extents.height, ctx->font->extents.height).quot + 1;

  if (!self->lines || (lines_len != self->lines_len)) {
    self->lines_len = lines_len;
    self->lines = realloc(self->lines, self->lines_len * sizeof(int));
    if (self->lines_len == 0) self->lines = NULL;
  }
}

bool input_iter_screen_xy(input_t *self, buff_string_iter_t *iter, int screen_x, int screen_y, bool x_adjust) {
  if (self->lines_len < 0) return false;
  __auto_type ctx = &self->ctx;
  int x = screen_x;
  int y = screen_y - ctx->clip.y;
  int line_y = div(y, ctx->font->extents.height).quot;
  int line_offset = self->lines[line_y];
  if (line_offset < 0) return false;
  cairo_text_extents_t extents;
  cairo_glyph_t glyph = {0,0,0};

  int current_x = ctx->clip.x;
  bs_index(&self->contents, iter, line_offset);
  bs_find(iter, lambda(bool _(char c){
    if (c == '\n') return true;
    glyph.index = c;
    cairo_glyph_extents(ctx->cairo, &glyph, 1, &extents);
    current_x += extents.x_advance;
    // TODO: better end of line detection
    if (current_x >= x) return true;
    return false;
  }));
  return true;
}

point_t selection_get_range(selection_t *selection, cursor_t *cursor) {
  point_t result;
  result.x
    = selection->state != Selection_Inactive ? bs_offset(&selection->mark_1)
    : -1;
  result.y
    = selection->state == Selection_Complete ? bs_offset(&selection->mark_2)
    : selection->state == Selection_DraggingKeyboard ? bs_offset(&cursor->pos)
    : selection->state == Selection_DraggingMouse ? bs_offset(&cursor->pos)
    : -1;
  if (result.x > result.y) {
    __auto_type temp = result.x;
    result.x = result.y;
    result.y = temp;
  }
  return result;
}

void input_line_selection_range(point_t *line_sel_range, point_t *sel_range, int begin_offset, int end_offset) {
  __auto_type line_len = end_offset - begin_offset;
  line_sel_range->x = -1;
  line_sel_range->y = -1;
  if (sel_range->x <= begin_offset && sel_range->y >= end_offset) {
    line_sel_range->x = 0;
    line_sel_range->y = line_len;
    return;
  }
  if (sel_range->x >= begin_offset && sel_range->x <= end_offset) {
    line_sel_range->x = sel_range->x - begin_offset;
    line_sel_range->y = MIN(sel_range->y - begin_offset, line_len);
  }
  if (sel_range->y >= begin_offset && sel_range->y <= end_offset) {
    line_sel_range->x = MAX(sel_range->x - begin_offset, 0);
    line_sel_range->y = sel_range->y - begin_offset;
  }
}

static void cursor_modified(input_t *self,cursor_t *prev) {
  int next_offset = bs_offset(&self->cursor.pos);
  int prev_offset = bs_offset(&prev->pos);
  int scroll_offset = bs_offset(&self->scroll.pos);
  int diff = next_offset - prev_offset;
  int screen_lines = div(self->ctx.clip.h, self->ctx.font->extents.height).quot;

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

void noop_highlight(void *self, highlighter_args_t *args, highlighter_t hl) {
  point_t range = {0, args->len};
  hl(range, args->normal);
}

static __attribute__((constructor)) void __init__() {
  noop_highlighter.reset = &noop;
  noop_highlighter.forward = &noop;
  noop_highlighter.highlight = &noop_highlight;
}
