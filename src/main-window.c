#include "main-window.h"

void main_window_init(main_window_t *self, widget_context_t *ctx, int argc, char **argv) {
  __auto_type path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];
  char dir_path[128];
  dir_path[dirname(path, dir_path)] = '\0';
  self->widget.ctx = ctx;
  self->widget.dispatch = (dispatch_t)&main_window_dispatch;
  self->show_sidebar = false;
  self->focus = (some_widget_t){(dispatch_t)&tabs_dispatch, (base_widget_t *)&self->content};
  self->hover = noop_widget;
  tabs_init(&self->content, ctx, path);
  tree_panel_init(&self->sidebar, ctx, dir_path);
  statusbar_init(&self->statusbar, ctx, NULL);
  self->statusbar.buffer = self->content.active ? &self->content.active->buffer : NULL;
}

void main_window_free(main_window_t *self) {
  tabs_free(&self->content);
  tree_panel_free(&self->sidebar);
}

void main_window_dispatch(main_window_t *self, main_window_msg_t *msg, yield_t yield) {
  auto some_widget_t lookup_children(lookup_filter_t filter);
  __auto_type ctx = self->widget.ctx;

  switch(msg->tag) {
  case Expose: {
    if (self->show_sidebar) {
      tree_panel_dispatch(&self->sidebar, (tree_panel_msg_t *)msg, &noop_yield);
    }
    tabs_dispatch(&self->content, (tabs_msg_t *)msg, &noop_yield);
    if (self->show_sidebar) {
      gx_set_color(ctx, ctx->palette->border);
      cairo_move_to(ctx->cairo, self->sidebar.widget.clip.x + self->sidebar.widget.clip.w, self->sidebar.widget.clip.y);
      cairo_line_to(ctx->cairo, self->sidebar.widget.clip.x + self->sidebar.widget.clip.w, self->sidebar.widget.clip.y + self->sidebar.widget.clip.h);
      cairo_stroke(ctx->cairo);
    }
    statusbar_dispatch(&self->statusbar, (statusbar_msg_t *)msg, &noop_yield);
    return;
  }
  case Widget_Free: {
    return main_window_free(self);
  }
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event->xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    __auto_type is_alt = xkey->state & Mod1Mask;
    if (keysym == XK_F4) {
      self->show_sidebar = !self->show_sidebar;
      main_window_msg_t next_msg = {.tag=Widget_Layout};
      yield(&next_msg);
      return yield(&msg_view);
    }
    if (keysym == XK_Up && is_alt) {
      return yield_children(tree_panel_dispatch, &self->sidebar, &(tree_panel_msg_t){.tag=TreePanel_Up});
    }
    break;
  }
  case Widget_Layout: {
    statusbar_msg_t measure = {.tag = Widget_Measure};
    statusbar_dispatch(&self->statusbar, &measure, &noop_yield);
    __auto_type sidebar_width = self->show_sidebar ? 280 : 0;
    rect_t content_clip = {self->widget.clip.x + sidebar_width, self->widget.clip.y, self->widget.clip.w - sidebar_width, self->widget.clip.h - measure.measure.y};
    rect_t tree_panel_clip = {self->widget.clip.x, self->widget.clip.y, sidebar_width, self->widget.clip.h - measure.measure.y};

    self->content.widget.clip = content_clip;
    self->sidebar.widget.clip = tree_panel_clip;

    self->statusbar.widget.clip.x = self->widget.clip.x;
    self->statusbar.widget.clip.y = content_clip.y + content_clip.h;
    self->statusbar.widget.clip.w = self->widget.clip.w;
    self->statusbar.widget.clip.h = measure.measure.y;

    yield_children(tabs_dispatch, &self->content, (tabs_msg_t *)msg);
    yield_children(tree_panel_dispatch, &self->sidebar, (tree_panel_msg_t *)msg);
    return;
  }
  case Widget_Children: {
    if (msg->widget.children.some.dispatch == (dispatch_t)&tree_panel_dispatch) {
      __auto_type child_msg = (tree_panel_msg_t *)msg->widget.children.msg;
      if (child_msg->tag == TreePanel_ItemClicked && child_msg->item_clicked->tag == Tree_File) {
        // Click on a file, open new tab
        tabs_msg_t next_msg = {.tag = Tabs_New, .new = {.path = child_msg->item_clicked->file.path}};
        return yield_children(tabs_dispatch, &self->content, &next_msg);
      }
    }
    __auto_type prev_active = self->content.active;
    dispatch_some(main_window_dispatch, msg->widget.children.some, msg->widget.children.msg);
    __auto_type next_active = self->content.active;
    if (prev_active != next_active) {
      self->statusbar.buffer = self->content.active ? &self->content.active->buffer : NULL;
      yield(&msg_view);
    }
    return;
  }}

  redirect_x_events(main_window_dispatch);

  some_widget_t lookup_children(lookup_filter_t filter) {
    switch(filter.tag) {
    case Lookup_Coords: {
      if (is_inside_point(self->statusbar.widget.clip, filter.coords)) {
        return (some_widget_t){(dispatch_t)&statusbar_dispatch, (base_widget_t *)&self->statusbar.widget};
      }
      if (is_inside_point(self->sidebar.widget.clip, filter.coords)) {
        return (some_widget_t){(dispatch_t)&tree_panel_dispatch, (base_widget_t *)&self->sidebar.widget};
      }
      if (is_inside_point(self->content.widget.clip, filter.coords)) {
        return (some_widget_t){(dispatch_t)&tabs_dispatch, (base_widget_t *)&self->content.widget};
      }
    }}
    return noop_widget;
  }
}
