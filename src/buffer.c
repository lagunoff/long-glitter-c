#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <cairo.h>

#include "buffer.h"
#include "graphics.h"
#include "utils.h"
#include "widget.h"
#include "c-mode.h"

void buffer_view_lines(buffer_t *self);
syntax_highlighter_t *buffer_choose_syntax_highlighter(char *path);

void buffer_init(buffer_t *self, widget_context_t *ctx, char *path) {
  struct stat st;
  self->fd = open(path, O_RDONLY);
  self->path = path;
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  self->widget.ctx = ctx;
  self->widget.dispatch = (dispatch_t)&buffer_dispatch;
  input_init(&self->input, ctx, new_bytes(mmaped, st.st_size), buffer_choose_syntax_highlighter(path));
  self->input.font = &self->font;
  self->input.highlight_line = true;
  self->show_lines = true;
  self->font = ctx->palette->monospace_font;
  self->modifier.state = 0;
  self->modifier.keysym = 0;
  gx_sync_font(&ctx->palette->monospace_font);
  self->focus = (some_widget_t){(dispatch_t)&input_dispatch, (base_widget_t *)&self->input};
  self->hover = noop_widget;
  address_line_init(&self->address_line, ctx, self);
}

void buffer_free(buffer_t *self) {
  gx_font_destroy(&self->font);
  input_free(&self->input);
  bs_free(self->input.contents, lambda(void _(bs_bytes_t *b){
    munmap(b->bytes, b->len);
  }));
  close(self->fd);
}

void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  auto some_widget_t lookup_children(lookup_filter_t filter);
  __auto_type ctx = self->widget.ctx;

  switch (msg->tag) {
  case Expose: {
    input_dispatch(&self->input, (input_msg_t *)&msg_view, &noop_yield);
    if (self->show_lines) buffer_view_lines(self);
    address_line_dispatch(&self->address_line, (address_line_msg_t *)&msg_view, &noop_yield);
    return;
  }
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event->xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    __auto_type is_ctrl = xkey->state & ControlMask;
    if (keysym == XK_F10) {
      self->show_lines = !self->show_lines;
      yield(&(widget_msg_t){.tag=Widget_Layout});
      return yield(&msg_view);
    }
    if (self->modifier.keysym == XK_x && (self->modifier.state & ControlMask) && keysym == XK_s && is_ctrl) {
      self->modifier.state = 0;
      self->modifier.keysym = 0;
      return yield(&(buffer_msg_t){.tag=Buffer_Save});
    }
    if (self->modifier.keysym == XK_x && (self->modifier.state & ControlMask) && keysym == XK_f && is_ctrl) {
      self->modifier.state = 0;
      self->modifier.keysym = 0;
      self->focus = (some_widget_t){(dispatch_t)&address_line_dispatch, (base_widget_t *)&self->address_line};
      return yield_children(address_line_dispatch, &self->address_line, &(widget_msg_t){.tag=Widget_FocusIn});
    }
    if (keysym == XK_Escape) {
      self->focus = (some_widget_t){(dispatch_t)&input_dispatch, (base_widget_t *)&self->input};
      return yield_children(input_dispatch, &self->input, &(widget_msg_t){.tag=Widget_FocusIn});
    }
    if (keysym == XK_x && is_ctrl) {
      self->modifier.state = xkey->state;
      self->modifier.keysym = keysym;
      return;
    }
    if (keysym == XK_minus && is_ctrl) {
      gx_font_destroy(&self->font);
      cairo_matrix_init_scale(&self->font.matrix, self->font.matrix.xx - 1, self->font.matrix.xx - 1);
      gx_sync_font(&self->font);
      gx_set_font(self->input.widget.ctx, &self->font);
      gx_set_font(ctx, &self->font);
      yield(&msg_layout);
      return yield(&msg_view);
    }
    if (keysym == XK_equal && is_ctrl) {
      gx_font_destroy(&self->font);
      cairo_matrix_init_scale(&self->font.matrix, self->font.matrix.xx + 1, self->font.matrix.xx + 1);
      gx_sync_font(&self->font);
      gx_set_font(self->input.widget.ctx, &self->font);
      gx_set_font(ctx, &self->font);
      yield(&msg_layout);
      return yield(&msg_view);
    }
    break;
  }
  case Widget_Free: {
    return buffer_free(self);
  }
  case Widget_Layout: {
    widget_msg_t measure_address_line = {.tag=Widget_Measure, .measure={INT_MAX, INT_MAX}};
    yield_children(address_line_dispatch, &self->address_line, &measure_address_line);

    self->address_line.widget.clip = (rect_t){
      self->widget.clip.x,
      self->widget.clip.y,
      MIN(measure_address_line.measure.x, self->widget.clip.w),
      MIN(measure_address_line.measure.y, self->widget.clip.h),
    };

    self->lines = (rect_t){
      self->widget.clip.x,
      self->widget.clip.y + self->address_line.widget.clip.h,
      self->input.font->extents.max_x_advance * 5,
      self->widget.clip.h - self->address_line.widget.clip.h,
    };

    self->input.widget.clip = (rect_t){
      self->lines.x + self->lines.w,
      self->widget.clip.y + self->address_line.widget.clip.h,
      self->widget.clip.w - self->lines.w,
      self->widget.clip.h - self->address_line.widget.clip.h,
    };

    yield_children(input_dispatch, &self->input, msg);
    yield_children(address_line_dispatch, &self->address_line, msg);
    return;
  }
  case Widget_Children: {
    if (msg->widget.children.some.dispatch == (dispatch_t)&input_dispatch) {
      __auto_type prev_offset = bs_offset(&self->input.cursor.pos);
      __auto_type prev_line = self->input.scroll.line;
      dispatch_some(buffer_dispatch, msg->widget.children.some, msg->widget.children.msg);
      __auto_type next_offset = bs_offset(&self->input.cursor.pos);
      __auto_type next_line = self->input.scroll.line;
      if (next_line != prev_line || prev_offset != next_offset) {
        buffer_view_lines(self);
      }
    }
    else if (msg->widget.children.some.dispatch == (dispatch_t)&menulist_dispatch) {
      __auto_type child_msg = (menulist_msg_t *)msg->widget.children.msg;
      if (child_msg->tag == Menulist_ItemClicked) {
        buffer_msg_t next_msg = {.tag = child_msg->item_clicked->action};
        yield(&next_msg);
        /* widget_close_window(self->context_menu.ctx.window); */
        /* self->context_menu.ctx.window = 0; */
        return;
      }
      if (child_msg->tag == Menulist_Destroy) {
        /* widget_close_window(self->context_menu.ctx.window); */
        /* self->context_menu.ctx.window = 0; */
        return;
      }
      dispatch_some(buffer_dispatch, msg->widget.children.some, msg->widget.children.msg);
      if (child_msg->tag == Expose) {
        // SDL_RenderPresent(self->context_menu.ctx.renderer);
      }
      return;
    }
    dispatch_some(buffer_dispatch, msg->widget.children.some, msg->widget.children.msg);
    return;
  }
  case Buffer_Save: {
    buff_string_iter_t iter;
    bs_begin(&iter, &self->input.contents);
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
        if (w == -1) return yield(&msg_view);
        written += w;
        if (written >= taken) break;
      }
      if (taken < buf_len) break;
    }
    close(fd);
    bs_free(self->input.contents, lambda(void _(bs_bytes_t *b){
      munmap(b->bytes, b->len);
    }));
    close(self->fd);
    struct stat st;
    self->fd = open(self->path, O_RDONLY);
    fstat(self->fd, &st);
    char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
    self->input.contents = new_bytes(mmaped, st.st_size);
    return yield(&msg_view);
  }}

  redirect_x_events(buffer_dispatch);

  some_widget_t lookup_children(lookup_filter_t filter) {
    return (some_widget_t){(dispatch_t)&input_dispatch, (base_widget_t *)&self->input};
  }
}

void buffer_view_lines(buffer_t *self) {
  __auto_type ctx = self->widget.ctx;
  __auto_type line = self->input.scroll.line;
  __auto_type color = ctx->palette->secondary_text;
  __auto_type cursor_offset = bs_offset(&self->input.cursor.pos);
  char temp[64];
  cairo_text_extents_t text_size;
  int y = self->lines.y;
  gx_set_color(self->widget.ctx, ctx->palette->white);
  gx_rect(self->widget.ctx, self->lines);

  gx_set_font(ctx, self->input.font);
  for(int i = 0; i < self->input.lines_len; line++, i++) {
    // File content ended, dont draw line numbers
    if (self->input.lines[i] == -1) break;
    __auto_type next_line_offset = i + 1 < self->input.lines_len ? self->input.lines[i + 1] : -1;
    __auto_type current_line = cursor_offset >= self->input.lines[i] && (next_line_offset == -1 || cursor_offset < next_line_offset);
    color.alpha = current_line ? 0.54 : 0.15;
    sprintf(temp, "%d", line + 1);
    gx_measure_text(ctx, temp, &text_size);
    gx_set_color(ctx, color);
    gx_text(ctx, self->lines.x + self->lines.w - 12 - text_size.x_advance, y + ctx->font->extents.ascent, temp);
    y += ctx->font->extents.height;
    if (y + ctx->font->extents.height >= self->lines.y + self->lines.h) break;
  }
}

syntax_highlighter_t *buffer_choose_syntax_highlighter(char *path) {
  __auto_type ext = extension(path); if (!ext) return &noop_highlighter;
  if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) return &c_mode_highlighter;
  return &noop_highlighter;
}
