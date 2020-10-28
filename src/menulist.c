#include <stdarg.h>
#include <stddef.h>
#include <SDL2/SDL.h>

#include "menulist.h"

void
menulist_init(menulist_t *self, menulist_item_t *items, int len, int alignement) {
  draw_init_font(&self->font, 12);
  self->ctx.font = &self->font;
  self->items = items;
  self->len = len;
  self->alignement = alignement;
  self->hover = -1;
}

void
menulist_free(menulist_t *self) {
  draw_free_font(self->ctx.font);
}

void
menulist_view(menulist_t *self) {
  draw_context_t *ctx = &self->ctx;
  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  draw_set_color(ctx, ctx->palette.ui_bg);
  SDL_RenderClear(ctx->renderer);
  int x_padding = 32;
  int y_margin = 8;
  int item_height = self->ctx.font->X_height + y_margin;
  int y = 1 + y_margin;
  draw_set_color(ctx, ctx->palette.primary_text);
  for (int i = 0; i < self->len; i++) {
    uint8_t *ptr8 = (uint8_t *)self->items;
    menulist_item_t *item_ptr = (menulist_item_t *)(ptr8 + i * self->alignement);
    if (i == self->hover) {
      draw_set_color(ctx, ctx->palette.selection_bg);
      draw_box(ctx, 0, y - 3, viewport.w, item_height);
      draw_set_color(ctx, ctx->palette.primary_text);
    }
    draw_text(ctx, x_padding + 1, y, item_ptr->title);
    y += item_height;
  }
  draw_set_color(ctx, ctx->palette.border);
  draw_rectangle(ctx, 0,0,viewport.w,viewport.h);
}

void
menulist_measure(menulist_t *self, SDL_Point *size) {
  int x_padding = 32;
  int y_margin = 8;
  int item_height = self->ctx.font->X_height + y_margin;
  size->x = 240;
  size->y = self->len * item_height + y_margin + 2;
}

bool
menulist_update(menulist_t *self, SDL_Event *e) {
  if (e->type == SDL_MOUSEMOTION) {
    SDL_Rect viewport;
    SDL_RenderGetViewport(self->ctx.renderer, &viewport);
    int y_margin = 8;
    int item_height = self->ctx.font->X_height + y_margin;
    int y = 1 + y_margin;
    for (int i = 0; i < self->len; i++) {
      if (e->motion.y - viewport.y >= y && e->motion.y - viewport.y < y + item_height) {
        self->hover = i;
        return true;
      }
      y += item_height;
    }
    self->hover = -1;
  }

  if (e->type == SDL_WINDOWEVENT) {
    if (e->window.event == SDL_WINDOWEVENT_LEAVE) {
      self->hover = -1;
      return true;
    }
  }

  return true;
}

menulist_action_t
menulist_context_update(menulist_t *self, SDL_Event *e) {
  if (e->type == SDL_WINDOWEVENT) {
    if (e->window.event == SDL_WINDOWEVENT_LEAVE || e->window.event == SDL_WINDOWEVENT_ENTER) {
      return MENULIST_NOOP;
    }
    return MENULIST_DESTROY;
  }
  if (e->type == SDL_MOUSEBUTTONDOWN) {
    return MENULIST_DESTROY;
  }
  return MENULIST_NOOP;
}

static __attribute__((constructor)) void __init__() {
  menulist_widget.update = (update_t)menulist_update;
  menulist_widget.view = (view_t)menulist_view;
  menulist_widget.measure = (measure_t)menulist_measure;
  menulist_widget.free = (finalizer_t)menulist_free;
}
