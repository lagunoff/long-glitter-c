#include "main-window.h"

void main_window_init(main_window_t *self, widget_context_init_t *ctx) {
  draw_init_context(&self->ctx, ctx);
  self->show_sidebar = true;
  tabs_init(&self->content, ctx, "/home/vlad/job/long-glitter-c/tmp/xola.c");
  tree_panel_init(&self->sidebar, ctx, "/home/vlad/job/long-glitter-c/tmp");
}

void main_window_free(main_window_t *self) {
  tabs_free(&self->content);
  tree_panel_free(&self->sidebar);
}

void main_window_dispatch(main_window_t *self, main_window_msg_t *msg, yield_t yield) {
  auto void yield_sidebar(tree_panel_msg_t *msg);
  auto void yield_content(tabs_msg_t *msg);
  __auto_type ctx = &self->ctx;
  switch(msg->tag) {
  case Expose: {
    if (self->show_sidebar) {
      tree_panel_dispatch(&self->sidebar, (tree_panel_msg_t *)msg, &noop_yield);
    }
    tabs_dispatch(&self->content, (tabs_msg_t *)msg, &noop_yield);
    if (self->show_sidebar) {
      draw_set_color(ctx, self->ctx.palette->border);
      cairo_move_to(ctx->cairo, self->sidebar.ctx.clip.x + self->sidebar.ctx.clip.w, self->sidebar.ctx.clip.y);
      cairo_line_to(ctx->cairo, self->sidebar.ctx.clip.x + self->sidebar.ctx.clip.w, self->sidebar.ctx.clip.y + self->sidebar.ctx.clip.h);
      cairo_stroke(ctx->cairo);
    }
    return;
  }
  case MSG_FREE: {
    return main_window_free(self);
  }
  case MotionNotify: {
    __auto_type motion = &msg->widget.x_event.xmotion;
    if (rect_is_inside(self->content.ctx.clip, motion->x, motion->y)) {
      return yield_content((tabs_msg_t *)msg);
    }
    if (rect_is_inside(self->sidebar.ctx.clip, motion->x, motion->y)) {
      return yield_sidebar((tree_panel_msg_t *)msg);
    }
    return;
  }
  case ButtonPress: {
    __auto_type button = &msg->widget.x_event.xbutton;
    if (rect_is_inside(self->content.ctx.clip, button->x, button->y)) {
      return yield_content((tabs_msg_t *)msg);
    }
    if (rect_is_inside(self->sidebar.ctx.clip, button->x, button->y)) {
      return yield_sidebar((tree_panel_msg_t *)msg);
    }
    return;
  }
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event.xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    if (keysym == XK_F4) {
      self->show_sidebar = !self->show_sidebar;
      buffer_msg_t next_msg = {.tag=MSG_LAYOUT};
      yield(&next_msg);
      return yield(&msg_view);
    }
    return yield_content((tabs_msg_t *)msg);
  }
  case SelectionRequest: {
    return yield_content((tabs_msg_t *)msg);
  }
  case SelectionNotify: {
    return yield_content((tabs_msg_t *)msg);
  }
  case MainWindow_Content: {
    return tabs_dispatch(&self->content, &msg->content, (yield_t)&yield_content);
  }
  case MainWindow_Sidebar: {
    return tree_panel_dispatch(&self->sidebar, &msg->sidebar, (yield_t)&yield_sidebar);
  }
  case MSG_LAYOUT: {
    __auto_type sidebar_width = self->show_sidebar ? 280 : 0;
    rect_t content_clip = {ctx->clip.x + sidebar_width, ctx->clip.y, ctx->clip.w - sidebar_width, ctx->clip.h};
    rect_t tree_panel_clip = {ctx->clip.x, ctx->clip.y, sidebar_width, ctx->clip.h};
    self->content.ctx.clip = content_clip;
    self->sidebar.ctx.clip = tree_panel_clip;
    yield_content((tabs_msg_t *)msg);
    yield_sidebar((tree_panel_msg_t *)msg);
    return;
  }}

  void yield_sidebar(tree_panel_msg_t *msg) {
    main_window_msg_t next_msg = {.tag=MainWindow_Sidebar, .sidebar=*msg};
    yield(&next_msg);
  }
  void yield_content(tabs_msg_t *msg) {
    main_window_msg_t next_msg = {.tag=MainWindow_Content, .content=*msg};
    yield(&next_msg);
  }
}
