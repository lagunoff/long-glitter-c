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
    if (item_ptr->title) {
      draw_set_color(ctx, palette.secondary_text);
      draw_glyph(ctx, 8, y - y_margin * 0.5 + (item_height - palette.fontawesome_font.ascent) * 0.5 - 1, item_ptr->icon, palette.fontawesome_font.font);
      draw_set_color(ctx, palette.primary_text);
    }
    draw_text(ctx, x_padding + 1, y, item_ptr->title);
    y += item_height;
  }
  draw_set_color(ctx, ctx->palette->border);
  draw_rectangle(ctx, 0, 0, viewport.w, viewport.h);
}

void
menulist_dispatch(menulist_t *self, menulist_msg_t *msg, yield_t yield) {
  switch (msg->tag) {
    case MSG_SDL_EVENT: {
      __auto_type e = msg->widget.sdl_event.event;
      if (e->type == SDL_MOUSEMOTION) {
        SDL_Rect viewport;
        SDL_RenderGetViewport(self->ctx.renderer, &viewport);
        int y_margin = 8;
        int item_height = self->ctx.font->X_height + y_margin;
        int y = 1 + y_margin;
        for (int i = 0; i < self->len; i++) {
          if (e->motion.y - viewport.y >= y && e->motion.y - viewport.y < y + item_height) {
            self->hover = i;
            goto _view;
          }
          y += item_height;
        }
        self->hover = -1;
      }
      if (e->type == SDL_MOUSEBUTTONUP) {
        SDL_Rect viewport;
        SDL_RenderGetViewport(self->ctx.renderer, &viewport);
        int y_margin = 8;
        int item_height = self->ctx.font->X_height + y_margin;
        int y = 1 + y_margin;
        for (int i = 0; i < self->len; i++) {
          if (e->motion.y - viewport.y >= y && e->motion.y - viewport.y < y + item_height) {
            __auto_type ptr8 = (uint8_t *)self->items;
            __auto_type item_ptr = (menulist_item_t *)(ptr8 + i * self->alignement);
            __auto_type clicked_msg = menulist_item_clicked(item_ptr);
            return yield(&clicked_msg);
          }
          y += item_height;
        }
        self->hover = -1;
      }
      if (e->type == SDL_WINDOWEVENT) {
        if (e->window.event == SDL_WINDOWEVENT_LEAVE) {
          self->hover = -1;
          goto _view;
        }
      }
      goto _view;
    }
    case MSG_FREE: {
      goto _noop;
    }
    case MSG_VIEW: {
      menulist_view(self);
      goto _noop;
    }
    case MSG_MEASURE: {
      int item_height = self->ctx.font->X_height + y_margin;
      msg->widget.measure.result->x = 200;
      msg->widget.measure.result->y = self->len * item_height + y_margin + 2;
      goto _noop;
    }
    case MENULIST_DESTROY: {
      goto _noop;
    }
    case MENULIST_PARENT_WINDOW: {
      __auto_type e = msg->parent_sdl_event;
      if (e->type == SDL_WINDOWEVENT) {
        if (e->window.event == SDL_WINDOWEVENT_LEAVE
            || e->window.event == SDL_WINDOWEVENT_ENTER
            || e->window.event == SDL_WINDOWEVENT_EXPOSED ) {
          goto _noop;
        }
        yield(&menulist_destroy);
        return;
      }
      if (e->type == SDL_MOUSEBUTTONDOWN) {
        if (e->button.button == SDL_BUTTON_LEFT) {
          yield(&menulist_destroy);
          return;
        }
      }
      goto _noop;
    }
  }

  _noop:
  return;
  _view:
  yield(&msg_view);
  return;
}

menulist_msg_t menulist_destroy = {.tag = MENULIST_DESTROY};

menulist_msg_t menulist_parent_window(SDL_Event *event) {
  menulist_msg_t msg = {.tag = MENULIST_PARENT_WINDOW, .parent_sdl_event = event};
  return msg;
}

menulist_msg_t menulist_item_clicked(menulist_item_t *item) {
  menulist_msg_t msg = {.tag = MENULIST_ITEM_CLICKED, .item_clicked = item};
  return msg;
}
