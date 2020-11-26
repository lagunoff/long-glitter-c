#include <stdarg.h>
#include <stddef.h>

#include "menulist.h"

static int X_PADDING = 32;
static int Y_MARGIN = 12;

void
menulist_init(menulist_t *self, menulist_item_t *items, int len, int alignement) {
  self->ctx.font = &self->ctx.palette->small_font;
  self->items = items;
  self->len = len;
  self->alignement = alignement;
  self->hover = -1;
}

void
menulist_free(menulist_t *self) {
}

void
menulist_view(menulist_t *self) {
  __auto_type ctx = &self->ctx;
  __auto_type extents = &self->ctx.font->extents;
  gx_set_color(ctx, ctx->palette->ui_bg);

  int item_height = extents->height + Y_MARGIN;
  int y = 1 + Y_MARGIN;
  gx_set_color(ctx, ctx->palette->primary_text);
  for (int i = 0; i < self->len; i++) {
    uint8_t *ptr8 = (uint8_t *)self->items;
    menulist_item_t *item_ptr = (menulist_item_t *)(ptr8 + i * self->alignement);
    if (i == self->hover) {
      gx_set_color(ctx, ctx->palette->hover);
      gx_box(ctx, 0, y - Y_MARGIN * 0.5, ctx->clip.w, item_height);
      gx_set_color(ctx, ctx->palette->primary_text);
    }
    if (item_ptr->icon) {
      gx_set_color(ctx, palette.secondary_text);
      // gx_glyph(ctx, 8, y - Y_MARGIN * 0.5 + (item_height - palette.fontawesome_font.ascent) * 0.5 - 1, item_ptr->icon, palette.fontawesome_font.font);
      gx_set_color(ctx, palette.primary_text);
    }
    gx_text(ctx, X_PADDING + 1, y, item_ptr->title);
    y += item_height;
  }
  gx_set_color(ctx, ctx->palette->border);
  gx_rectangle(ctx, 0, 0, ctx->clip.w, ctx->clip.h);
}

void
menulist_dispatch(menulist_t *self, menulist_msg_t *msg, yield_t yield) {
  switch (msg->tag) {
  case Expose: {
    return menulist_view(self);
  }
  case MotionNotify: {
    __auto_type ctx = &self->ctx;
    __auto_type extents = &ctx->font->extents;
    __auto_type prev_hover = self->hover;
    __auto_type xmotion = &msg->widget.x_event->xmotion;

    int Y_MARGIN = 8;
    int item_height = extents->height + Y_MARGIN;
    int y = 1 + Y_MARGIN;
    for (int i = 0; i < self->len; i++) {
      if (xmotion->y - ctx->clip.y >= y && xmotion->y - ctx->clip.y < y + item_height) {
        self->hover = i;
        goto check_hover;
      }
      y += item_height;
    }
    self->hover = -1;
    check_hover:
    if (self->hover != prev_hover)
      yield(&msg_view);
    return;
  }
  case LeaveNotify: {
    self->hover = -1;
    return yield(&msg_view);
  }
  case Widget_Free: {
  }
  case Widget_Measure: {
    __auto_type extents = &self->ctx.font->extents;
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
}
