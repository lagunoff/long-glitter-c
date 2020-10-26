#include <stdlib.h>
#include <stdio.h>
#include <SDL2_gfxPrimitives.h>

#include "draw.h"

typedef union {
  SDL_Color color;
  Uint32 uint32;
} color_to_uint32_t;

void draw_text(draw_context_t *ctx, int x, int y, char *text) {
  if (text[0] == '\0') return;
  SDL_Rect rect;
  SDL_Surface *surface = TTF_RenderUTF8_Blended(ctx->font->font, text, ctx->foreground);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
  rect.x = x;
  rect.y = y;
  rect.w = surface->w;
  rect.h = surface->h;
  SDL_FreeSurface(surface);
  SDL_RenderCopy(ctx->renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
}

void draw_init_font(draw_font_t *self, int font_size) {
  self->font_size = font_size;
  self->font = TTF_OpenFont("/home/vlad/downloads/ttf/Hack-Regular.ttf", font_size);
  TTF_SizeText(self->font, "X", &self->X_width, &self->X_height);
  if (self->font == NULL) {
    fprintf(stderr, "Cannot open font file\n");
    exit(1);
  }
}

SDL_Color draw_rgba(double r, double g, double b, double a) {
  SDL_Color color = {r*255, g*255, b*255, a*255};
  return color;
}

void draw_init_palette(draw_palette_t *self) {
  self->primary_text = draw_rgba(0,0,0,0.87);
  self->current_line_bg = draw_rgba(0.0, 0.0, 0.6, 0.05);
  self->selection_bg = draw_rgba(0.8, 0.87, 0.98, 1);
}

void draw_init_context(draw_context_t *self, SDL_Renderer *renderer, draw_font_t *font) {
  draw_init_palette(&self->palette);
  self->renderer = renderer;
  self->font = font;
  self->background = draw_rgba(1,1,1,1);
  self->foreground = self->palette.primary_text;
}

void draw_set_color(draw_context_t *ctx, SDL_Color color) {
  ctx->foreground = color;
  SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
}

void draw_measure_text(draw_context_t *ctx, char *text, SDL_Point *size) {
  TTF_SizeUTF8(ctx->font->font, text, &size->x, &size->y);
}

void draw_rectangle(draw_context_t *ctx, Sint16 x, Sint16 y, Sint16 w, Sint16 h) {
  color_to_uint32_t ctu = {.color = ctx->foreground};
  rectangleColor(ctx->renderer, x, y, x + w, y + h, ctu.uint32);
}

void draw_box(draw_context_t *ctx, Sint16 x, Sint16 y, Sint16 w, Sint16 h) {
  color_to_uint32_t ctu = {.color = ctx->foreground};
  boxColor(ctx->renderer, x, y, x + w, y + h, ctu.uint32);
}

void draw_set_color_rgba(draw_context_t *ctx, double r, double g, double b, double a) {
  draw_set_color(ctx, draw_rgba(r, g, b, a));
}
