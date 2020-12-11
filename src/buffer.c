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
  self->widget = (widget_container_t){
    Widget_Container, {0,0,0,0}, ctx,
    ((dispatch_t)&buffer_dispatch), NULL, NULL
  };
  struct stat st;
  self->fd = open(path, O_RDONLY);
  self->path = path;
  fstat(self->fd, &st);
  char *mmaped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, self->fd, 0);
  input_init(&self->input, ctx, new_bytes(mmaped, st.st_size), buffer_choose_syntax_highlighter(path));
  self->input.font = &self->font;
  self->input.highlight_line = true;
  self->show_lines = true;
  self->font = ctx->palette->monospace_font;
  self->modifier.state = 0;
  self->modifier.keysym = 0;
  self->lines.tag = Widget_Rectangle;
  gx_sync_font(&ctx->palette->monospace_font);
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

void buffer_dispatch_(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
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
      self->widget.focus = coerce_widget(&self->address_line.widget);
      return dispatch_to(yield, &self->address_line, &(widget_msg_t){.tag=Widget_FocusIn});
    }
    if (keysym == XK_Escape || (keysym == XK_g && is_ctrl)) {
      self->widget.focus = coerce_widget(&self->input.widget);
      dispatch_to(yield, &self->input, &(widget_msg_t){.tag=Widget_FocusIn});
      break;
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
    dispatch_to(yield, &self->address_line, &measure_address_line);

    self->address_line.widget.clip = (rect_t){
      self->widget.clip.x,
      self->widget.clip.y,
      MIN(measure_address_line.measure.x, self->widget.clip.w),
      MIN(measure_address_line.measure.y, self->widget.clip.h),
    };

    self->lines.clip = (rect_t){
      self->widget.clip.x,
      self->widget.clip.y + self->address_line.widget.clip.h,
      self->input.font->extents.max_x_advance * 5,
      self->widget.clip.h - self->address_line.widget.clip.h,
    };

    self->input.widget.clip = (rect_t){
      self->lines.clip.x + self->lines.clip.w,
      self->widget.clip.y + self->address_line.widget.clip.h,
      self->widget.clip.w - self->lines.clip.w,
      self->widget.clip.h - self->address_line.widget.clip.h,
    };

    dispatch_to(yield, &self->input, msg);
    dispatch_to(yield, &self->address_line, msg);
    return;
  }
  case Widget_Lookup: {
    msg->widget.lookup.response = coerce_widget(&self->input);
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
}

void buffer_dispatch(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
  void dispatch_step_1(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
    sync_container(self, msg, yield, (dispatch_t)buffer_dispatch_);
  }
  void dispatch_step_2(buffer_t *self, buffer_msg_t *msg, yield_t yield) {
    __auto_type next = &dispatch_step_1;
    if (msg->tag == Widget_NewChildren) {
      if (msg->widget.new_children.widget == coerce_widget(&self->input)) {
        __auto_type prev_offset = bs_offset(&self->input.cursor.pos);
        __auto_type prev_line = bs_offset(&self->input.scroll.pos);
        dispatch_to(yield, msg->widget.new_children.widget, msg->widget.new_children.msg);
        __auto_type next_offset = bs_offset(&self->input.cursor.pos);
        __auto_type next_line = bs_offset(&self->input.scroll.pos);
        if ((next_line != prev_line) || (prev_offset != next_offset)) {
          buffer_view_lines(self);
        }
      }
      else if (msg->widget.new_children.widget == coerce_widget(&self->context_menu.widget)) {
        __auto_type child_msg = (menulist_msg_t *)msg->widget.new_children.msg;
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
        dispatch_to(yield, msg->widget.new_children.widget, msg->widget.new_children.msg);
        if (child_msg->tag == Expose) {
          // SDL_RenderPresent(self->context_menu.ctx.renderer);
        }
        return;
      } else {
        dispatch_to(yield, msg->widget.new_children.widget, msg->widget.new_children.msg);
      }
      return;
    } else {
      return next(self, msg, yield);
    }
  }

  return sync_container(self, msg, yield, (dispatch_t)dispatch_step_1);
}

void buffer_view_lines(buffer_t *self) {
  __auto_type ctx = self->widget.ctx;
  __auto_type line = self->input.scroll.line;
  __auto_type color = ctx->palette->secondary_text;
  __auto_type cursor_offset = bs_offset(&self->input.cursor.pos);
  char temp[64];
  cairo_text_extents_t text_size;
  int y = self->lines.clip.y;
  gx_set_color(self->widget.ctx, ctx->palette->white);
  gx_rect(self->widget.ctx, self->lines.clip);

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
    gx_text(ctx, self->lines.clip.x + self->lines.clip.w - 12 - text_size.x_advance, y + ctx->font->extents.ascent, temp);
    y += ctx->font->extents.height;
    if (y + ctx->font->extents.height >= self->lines.clip.y + self->lines.clip.h) break;
  }
}

syntax_highlighter_t *buffer_choose_syntax_highlighter(char *path) {
  __auto_type ext = extension(path); if (!ext) return &noop_highlighter;
  if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) return &c_mode_highlighter;
  return &noop_highlighter;
}
