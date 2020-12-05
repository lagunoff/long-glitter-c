#include "tabs.h"
#include "dlist.h"
#include "utils.h"

void tabs_init(tabs_t *self, widget_context_t *ctx, char *path) {
  self->widget.ctx = ctx;
  self->widget.dispatch = (dispatch_t)&tabs_dispatch;
  self->active = malloc(sizeof(buffer_list_node_t));
  self->active->next = NULL;
  self->active->prev = NULL;
  buffer_init(&self->active->buffer, ctx, path);
  self->tabs.first = self->active;
  self->tabs.last = self->active;
  self->focus = (some_widget_t){(dispatch_t)&buffer_dispatch, (base_widget_t *)&self->active->buffer};
  self->hover = noop_widget;
}

void tabs_free(tabs_t *self) {
  for(__auto_type iter = self->tabs.first; iter;) {
    __auto_type next_iter = iter->next;
    buffer_free(&iter->buffer);
    free(iter);
    iter = next_iter;
  }
}

void tabs_dispatch(tabs_t *self, tabs_msg_t *msg, yield_t yield) {
  auto void title_dispatch(base_widget_t *self, widget_msg_t *msg, yield_t yield);
  auto some_widget_t lookup_children(lookup_filter_t filter);

  __auto_type ctx = self->widget.ctx;
  buffer_list_node_t *current_inst = NULL;

  redirect_x_events(tabs_dispatch);

  switch(msg->tag) {
  case Expose: {
    return tabs_view(self);
  }
  case Widget_Free: {
    return tabs_free(self);
  }
  case ButtonPress: {
    __auto_type button = &msg->widget.x_event->xbutton;
    if (is_inside_xy(self->tabs_clip, button->x, button->y)) {
      switch (button->button) {
      case Button1: { // Left click
        for(__auto_type iter = self->tabs.first; iter; iter = iter->next) {
          if (is_inside_xy(iter->title.clip, button->x, button->y)) {
            tabs_msg_t next_msg = {.tag = Tabs_TabClicked, .tab_clicked = iter};
            return yield(&next_msg);
          }
        }
        return;
      }
      case Button4: { // Wheel up
        tabs_msg_t next_msg = {.tag = Tabs_Prev};
        return yield(&next_msg);
      }
      case Button5: { // Wheel down
        return yield(&(tabs_msg_t){.tag=Tabs_Next});
      }}
    };
    return dispatch_some(tabs_dispatch, self->focus, msg);
  }
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event->xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    __auto_type is_ctrl = xkey->state & ControlMask;
    __auto_type is_shift = xkey->state & ShiftMask;

    if (keysym == XK_w && is_ctrl && is_shift) {
      tabs_msg_t next_msg = {.tag=Tabs_Close, .close=self->active};
      return yield(&next_msg);
    }
    if (keysym == XK_Page_Down && is_ctrl) {
      tabs_msg_t next_msg = {.tag = Tabs_Prev};
      return yield(&next_msg);
    }
    if (keysym == XK_Page_Up && is_ctrl) {
      tabs_msg_t next_msg = {.tag = Tabs_Next};
      return yield(&next_msg);
    }
    break;
  }
  case Widget_Layout: {
    __auto_type tabs_height = 36;
    __auto_type iter = self->tabs.first;
    self->content_clip.x = self->widget.clip.x;
    self->content_clip.y = self->widget.clip.y + tabs_height;
    self->content_clip.w = self->widget.clip.w;
    self->content_clip.h = self->widget.clip.h - tabs_height;
    for(; iter; iter = iter->next) {
      iter->buffer.widget.clip = self->content_clip;
      yield(&(widget_msg_t){
        .tag=Widget_Children,
        .children={{(dispatch_t)&buffer_dispatch, (base_widget_t *)&iter->buffer}, msg}
      });
    }
    self->tabs_clip.x = self->widget.clip.x; self->tabs_clip.y = self->widget.clip.y;
    self->tabs_clip.w = self->widget.clip.w; self->tabs_clip.h = tabs_height;
    return;
  }
  case Tabs_New: {
    // First check if the file already opened, if so, focus on its tab
    for(__auto_type iter = self->tabs.first; iter; iter = iter->next) {
      if (strcmp(iter->buffer.path, msg->new.path) == 0) {
        return yield(&(tabs_msg_t){.tag=Tabs_TabClicked, .tab_clicked=iter});
      }
    }
    __auto_type new_tab = (buffer_list_node_t *)malloc(sizeof(buffer_list_node_t));
    buffer_init(&new_tab->buffer, (void *)ctx, msg->new.path);
    new_tab->buffer.widget.clip = self->content_clip;
    buffer_msg_t next_msg = {.tag=Widget_Layout};
    buffer_dispatch(&new_tab->buffer, &next_msg, &noop_yield);
    dlist_insert_after((dlist_head_t *)&self->tabs, (dlist_node_t *)new_tab, (dlist_node_t *)self->tabs.last);
    self->active = new_tab;
    return yield(&msg_view);
  }
  case Tabs_Close: {
    dlist_delete((dlist_head_t *)&self->tabs, (dlist_node_t *)msg->close);
    buffer_free(&msg->close->buffer);
    free(msg->close);
    self->active = msg->close == self->active ? self->tabs.first : self->active;
    return yield(&msg_view);
  }
  case Tabs_TabClicked: {
    self->active = msg->tab_clicked;
    return yield(&msg_view);
  }
  case Tabs_Prev: {
    if (!(self->active) || !(self->active->prev)) return;
    self->active = self->active->prev;
    return yield(&msg_view);
  }
  case Tabs_Next: {
    if (!(self->active) || !(self->active->next)) return;
    self->active = self->active->next;
    return yield(&msg_view);
  }}

  some_widget_t lookup_children(lookup_filter_t filter) {
    switch(filter.tag) {
    case Lookup_Coords: {
      if (self->active && is_inside_point(self->active->buffer.widget.clip, filter.coords)) {
        return (some_widget_t){(dispatch_t)&buffer_dispatch, (base_widget_t *)&self->active->buffer.widget};
      }
      for(__auto_type iter = self->tabs.first; iter; iter = iter->next) {
        if (is_inside_point(iter->title.clip, filter.coords)) {
          return (some_widget_t){(dispatch_t)&title_dispatch, &iter->title};
        }
      }
    }}
    return noop_widget;
  }

  void title_dispatch(base_widget_t *self, widget_msg_t *msg, yield_t yield) {
    switch(msg->tag) {
    case Widget_MouseEnter: {

    }
    case Widget_MouseLeave: {
    }}
  }
}

void tabs_view(tabs_t *self) {
  __auto_type ctx = self->widget.ctx;
  // Draw contents
  if (self->active) {
    buffer_dispatch(&self->active->buffer, (buffer_msg_t *)&msg_view, &noop_yield);
  }
  // Draw tabs
  gx_set_color_rgba(ctx, 0.97, 0.97, 0.97, 1);
  gx_set_font(ctx, ctx->font);
  gx_rect(ctx, self->tabs_clip);
  int x = self->tabs_clip.x;
  int y_text = self->tabs_clip.y + (self->tabs_clip.h - ctx->font->extents.ascent) * 0.5 + ctx->font->extents.ascent;
  for(__auto_type iter = self->tabs.first; iter; iter = iter->next) {
    char *fname = basename(iter->buffer.path);
    int x_padding = 16;
    cairo_text_extents_t extents;
    gx_measure_text(ctx, fname, &extents);
    if (self->active == iter) {
      gx_set_color_rgba(ctx, 1, 1, 1, 1);
    } else {
      gx_set_color_rgba(ctx, 0.95, 0.95, 0.95, 1);
    }
    gx_rectangle(ctx, x, self->tabs_clip.y, extents.x_advance + x_padding * 2, self->tabs_clip.h);
    gx_set_color_rgba(ctx, 0.97, 0.97, 0.97, 1);
    gx_line(ctx, x + extents.x_advance + x_padding * 2, self->tabs_clip.y, x + extents.x_advance + x_padding * 2, self->tabs_clip.y + self->tabs_clip.h);
    gx_set_color(ctx, ctx->palette->secondary_text);
    gx_text(ctx, x + x_padding, y_text, fname);
    iter->title.clip.x = x;
    iter->title.clip.y = self->tabs_clip.y;
    iter->title.clip.w = extents.x_advance + x_padding * 2;
    iter->title.clip.h = self->tabs_clip.h;
    x += extents.x_advance + x_padding * 2;
  }
}
