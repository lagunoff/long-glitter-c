#include "main-window.h"

void main_window_init(main_window_t *self, widget_context_t *ctx, int argc, char **argv) {
  __auto_type path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];
  char dir_path[128];
  dir_path[dirname(path, dir_path)] = '\0';
  self->widget = (widget_container_t){
    Widget_Container, {0,0,0,0}, ctx,
    ((dispatch_t)&main_window_dispatch),
    NULL, coerce_widget(&self->content.widget)
  };
  self->show_sidebar = false;
  tabs_init(&self->content, ctx, path);
  tree_panel_init(&self->sidebar, ctx, dir_path);
  statusbar_init(&self->statusbar, ctx, NULL);
  self->statusbar.buffer = self->content.active ? &self->content.active->buffer : NULL;
}

void main_window_free(main_window_t *self) {
  tabs_free(&self->content);
  tree_panel_free(&self->sidebar);
}

void main_window_dispatch_(main_window_t *self, main_window_msg_t *msg, yield_t yield, dispatch_t next) {
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
      return dispatch_to(yield, &self->sidebar, &(tree_panel_msg_t){.tag=TreePanel_Up});
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

    dispatch_to(yield, &self->content, (tabs_msg_t *)msg);
    dispatch_to(yield, &self->sidebar, (tree_panel_msg_t *)msg);
    return;
  }
  case Widget_Lookup: {
    __auto_type filter = msg->widget.lookup.request;
    switch(filter.tag) {
    case Lookup_Coords: {
      if (is_inside_point(self->statusbar.widget.clip, filter.coords)) {
        msg->widget.lookup.response = coerce_widget(&self->statusbar.widget);
        return;
      }
      if (is_inside_point(self->sidebar.widget.clip, filter.coords)) {
        msg->widget.lookup.response = coerce_widget(&self->sidebar.widget);
        return;
      }
      if (is_inside_point(self->content.widget.clip, filter.coords)) {
        msg->widget.lookup.response = coerce_widget(&self->content.widget);
        return;
      }
    }}
    return;
  }
  case Widget_NewChildren: {
      if (msg->widget.new_children.widget == coerce_widget(&self->sidebar.widget)) {
        __auto_type child_msg = (tree_panel_msg_t *)msg->widget.new_children.msg;
        if (child_msg->tag == TreePanel_ItemClicked && child_msg->item_clicked->tag == Tree_File) {
          // Click on a file, open new tab
          tabs_msg_t next_msg = {.tag = Tabs_New, .new = {.path = child_msg->item_clicked->file.path}};
          return dispatch_to(yield, &self->content, &next_msg);
        }
      }
      __auto_type prev_active = self->content.active;
      next(self, msg, yield);
      __auto_type next_active = self->content.active;
      if (prev_active != next_active) {
        self->statusbar.buffer = self->content.active ? &self->content.active->buffer : NULL;
        yield(&msg_view);
      }
      return;
  }}
  return next(self, msg, yield);
}

void main_window_dispatch(main_window_t *self, main_window_msg_t *msg, yield_t yield) {
  void step_1(main_window_t *self, main_window_msg_t *msg, yield_t yield) {
    return sync_container(self, msg, yield, &noop_dispatch);
  }
  return main_window_dispatch_(self, msg, yield, (dispatch_t)&step_1);
}
