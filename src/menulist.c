#include <stdarg.h>
#include <stddef.h>
#include <SDL2/SDL.h>

#include "menulist.h"

static int x_padding = 32;
static int y_margin = 12;

void
menulist_init(menulist_t *self, menulist_item_t *items, int len, int alignement) {
  self->ctx.font = &palette.small_font;
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
  draw_context_t *ctx = &self->ctx;
  draw_font_t *initial_font = ctx->font;
  SDL_Rect viewport;
  SDL_RenderGetViewport(ctx->renderer, &viewport);
  draw_set_color(ctx, ctx->palette->ui_bg);
  SDL_RenderClear(ctx->renderer);
  int item_height = self->ctx.font->X_height + y_margin;
  int y = 1 + y_margin;
  draw_set_color(ctx, ctx->palette->primary_text);
  for (int i = 0; i < self->len; i++) {
    uint8_t *ptr8 = (uint8_t *)self->items;
    menulist_item_t *item_ptr = (menulist_item_t *)(ptr8 + i * self->alignement);
    if (i == self->hover) {
      draw_set_color(ctx, ctx->palette->hover);
      draw_box(ctx, 0, y - y_margin * 0.5, viewport.w, item_height);
      draw_set_color(ctx, ctx->palette->primary_text);
    }
    int glyph = -1;
    if (strcmp(item_ptr->title, "Cut") == 0) glyph = 0xf0c4;
    if (strcmp(item_ptr->title, "Copy") == 0) glyph = 0xf328;
    if (strcmp(item_ptr->title, "Paste") == 0) glyph = 0xf15c;
    if (glyph != -1) {
      draw_set_color(ctx, palette.secondary_text);
      draw_glyph(ctx, 8, y - 3, glyph, palette.fontawesome_font.font);
      draw_set_color(ctx, palette.primary_text);
    }
    draw_text(ctx, x_padding + 1, y, item_ptr->title);
    y += item_height;
  }
  draw_set_color(ctx, ctx->palette->border);
  draw_rectangle(ctx, 0, 0, viewport.w, viewport.h);
}

void
menulist_measure(menulist_t *self, SDL_Point *size) {
  int item_height = self->ctx.font->X_height + y_margin;
  size->x = 200;
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
