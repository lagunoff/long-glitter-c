#include <stdarg.h>
#include <stddef.h>

#include "menulist.h"

static int X_PADDING = 32;
static int Y_MARGIN = 12;
static void menulist_focus_cycle(menulist_t *self, int n);

void menulist_init(menulist_t *self, widget_context_t *ctx, menulist_item_t *items, int len, int alignement) {
  self->widget = (widget_container_t){
    Widget_Container, {0,0,0,0}, ctx,
    (dispatch_t)&menulist_dispatch, NULL, NULL
  };
  self->items = items;
  self->len = len;
  self->alignement = alignement;
}

void menulist_free(menulist_t *self) {
}

void menulist_dispatch_(menulist_t *self, menulist_msg_t *msg, yield_t yield, dispatch_t next) {
  __auto_type ctx = self->widget.ctx;

  switch (msg->tag) {
  case Expose: {
    return menulist_view(self);
  }
  case KeyPress: {
    __auto_type xkey = &msg->widget.x_event->xkey;
    __auto_type keysym = XLookupKeysym(xkey, 0);
    __auto_type is_ctrl = xkey->state & ControlMask;
    if (keysym == XK_Escape || (keysym == XK_g && is_ctrl)) {
      return yield(&(menulist_msg_t){.tag=Menulist_Destroy});
    }
    if (keysym == XK_Down) {
      menulist_focus_cycle(self, 1);
      return yield(&msg_view);
    }
    if (keysym == XK_Up) {
      menulist_focus_cycle(self, -1);
      return yield(&msg_view);
    }
    return;
  }
  case Widget_Free: {
  }
  case Widget_Measure: {
    __auto_type extents = &ctx->palette->small_font.extents;
    int item_height = extents->height + Y_MARGIN;
    // TODO: measure max text width
    msg->widget.measure.x = 200;
    msg->widget.measure.y = self->len * item_height + Y_MARGIN + 2;
    return;
  }
  case Menulist_Destroy: {
    return;
  }
  case Menulist_DispatchParent: {
    __auto_type e = msg->parent_x_event;
    if (e->type == FocusOut || e->type == ButtonPress) {
      menulist_msg_t msg = {.tag = Menulist_Destroy};
      return yield(&msg);
    }
  }}

  return next(self, msg, yield);
}

void menulist_dispatch(menulist_t *self, menulist_msg_t *msg, yield_t yield) {
  void step_1(menulist_t *self, menulist_msg_t *msg, yield_t yield) {
    return sync_container(self, msg, yield, &noop_dispatch);
  }
  return menulist_dispatch_(self, msg, yield, (dispatch_t)&step_1);
}

void menulist_view(menulist_t *self) {
  __auto_type ctx = self->widget.ctx;
  __auto_type extents = &ctx->palette->small_font.extents;
  gx_set_color(ctx, gx_rgba(1,1,1,1));
  gx_rect(ctx, self->widget.clip);

  int item_height = extents->height + Y_MARGIN;
  int y = 1 + Y_MARGIN;
  gx_set_color(ctx, ctx->palette->primary_text);
  gx_set_font(ctx, ctx->font);
  for (int i = 0; i < self->len; i++) {
    uint8_t *ptr8 = (uint8_t *)self->items;
    menulist_item_t *item_ptr = (menulist_item_t *)(ptr8 + i * self->alignement);
    item_ptr->widget.clip = (rect_t){0, y - Y_MARGIN * 0.5, self->widget.clip.w, item_height};
    if (
      coerce_widget(&item_ptr->widget) == self->widget.hover ||
      coerce_widget(&item_ptr->widget) == self->widget.focus
    ) {
      gx_set_color(ctx, ctx->palette->hover);
      gx_box(ctx, 0, y - Y_MARGIN * 0.5, self->widget.clip.w, item_height);
      gx_set_color(ctx, ctx->palette->primary_text);
    }
    if (item_ptr->icon) {
      gx_set_color(ctx, palette.secondary_text);
      // gx_glyph(ctx, 8, y - Y_MARGIN * 0.5 + (item_height - palette.fontawesome_font.ascent) * 0.5 - 1, item_ptr->icon, palette.fontawesome_font.font);
      gx_set_color(ctx, palette.primary_text);
    }
    gx_text(ctx, X_PADDING + 1, y - Y_MARGIN * 0.5 + (item_height - ctx->font->extents.ascent) * 0.5 + ctx->font->extents.ascent, item_ptr->title);
    y += item_height;
  }
  gx_set_color(ctx, ctx->palette->border);
  gx_rectangle(ctx, 0, 0, self->widget.clip.w, self->widget.clip.h);
}

static void menulist_focus_cycle(menulist_t *self, int n) {
  if (!self->widget.focus) {
    if (self->len) self->widget.focus = (new_widget_t*)&self->items[0];
    return;
  }
  __auto_type idx = ((menulist_item_t*)self->widget.focus - &self->items[0]);
  idx += n;
  __auto_type new_idx = div(idx, self->len).rem;
  self->widget.focus = (new_widget_t*)&self->items[new_idx];
}
