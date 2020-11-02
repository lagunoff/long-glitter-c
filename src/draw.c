#include <stdlib.h>
#include <stdio.h>
#include <SDL2_gfxPrimitives.h>

#include "draw.h"
#include "widget.h"

typedef union {
  SDL_Color color;
  Uint32 uint32;
} color_to_uint32_t;

struct hex_color {
  char r1;
  char r2;
  char g1;
  char g2;
  char b1;
  char b2;
} __attribute__((packed));

void draw_text(draw_context_t *ctx, int x, int y, char *text) {
  if (text[0] == '\0') return;
  SDL_Rect rect;
  SDL_Surface *surface = TTF_RenderUTF8_Blended(ctx->font->font, text, ctx->foreground);
  if (!surface) {
    fprintf(stderr, SDL_GetError());
    exit(1);
  }
  SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
  if (!texture) {
    fprintf(stderr, SDL_GetError());
    exit(1);
  }
  rect.x = x;
  rect.y = y;
  rect.w = surface->w;
  rect.h = surface->h;
  SDL_FreeSurface(surface);
  SDL_RenderCopy(ctx->renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
}

void draw_glyph(draw_context_t *ctx, int x, int y, int ch, TTF_Font *font) {
  SDL_Rect rect;
  SDL_Surface *surface = TTF_RenderGlyph_Blended(font, ch, ctx->foreground);
  if (!surface) {
    fprintf(stderr, SDL_GetError());
    exit(1);
  }
  SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
  if (!texture) {
    fprintf(stderr, SDL_GetError());
    exit(1);
  }
  rect.x = x;
  rect.y = y;
  rect.w = surface->w;
  rect.h = surface->h;
  SDL_FreeSurface(surface);
  SDL_RenderCopy(ctx->renderer, texture, NULL, &rect);
  SDL_DestroyTexture(texture);
}

void draw_init_font(draw_font_t *self, char *path, int font_size) {
  self->font_size = font_size;
  self->font = TTF_OpenFont(path, font_size);
  int unused;
  TTF_SizeText(self->font, "X", &self->X_width, &unused);
  self->X_height = TTF_FontLineSkip(self->font);
  self->ascent = TTF_FontAscent(self->font);
  if (self->font == NULL) {
    fprintf(stderr, "Cannot open font file\n");
    exit(1);
  }
}

SDL_Color draw_rgba(double r, double g, double b, double a) {
  SDL_Color color = {r*255, g*255, b*255, a*255};
  return color;
}

void draw_init_context(draw_context_t *self, draw_font_t *font) {
  self->palette = &palette;
  self->font = font;
  self->background = draw_rgba(1,1,1,1);
  self->foreground = self->palette->primary_text;
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

void draw_open_window(
  SDL_Rect      *size_pos,
  Uint32         window_flags,
  SDL_Window   **window,
  SDL_Renderer **renderer,
  widget_t      *widget,
  void          *model
) {
  if (SDL_CreateWindowAndRenderer(size_pos->w, size_pos->h, SDL_WINDOW_TOOLTIP, window, renderer) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  SDL_SetWindowPosition(*window, size_pos->x, size_pos->y);
  widget_window_set(*window, widget, model);
  SDL_ShowWindow(*window);
}

void draw_close_window(SDL_Window *window) {
  widget_t *root_widget = NULL;
  void *root_widget_data = NULL;
  SDL_Renderer *renderer = SDL_GetRenderer(window);
  widget_window_get(window, &root_widget, &root_widget_data);
  if (root_widget && root_widget_data && root_widget->free) {
    root_widget->free(root_widget_data);
  }
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void draw_open_window_measure(
  SDL_Point       *pos,
  Uint32           window_flags,
  SDL_Window     **window,
  SDL_Renderer   **renderer,
  struct widget_t *widget,
  void            *model
) {
  SDL_Point measured;
  widget->measure(model, &measured);
  SDL_Rect rect = {pos->x, pos->y, measured.x, measured.y};
  draw_open_window(&rect,window_flags, window, renderer, widget, model);
}

void draw_free_font(draw_font_t *self) {
  TTF_CloseFont(self->font);
}

SDL_Color draw_rgb_hex(char *str) {
  int hexval(char c) {
    return c >= '0' && c <= '9' ? c - '0' : tolower(c) - 'a' + 10;
  }

  __auto_type view_hex = (struct hex_color *)str;
  SDL_Color color;
  color.r = hexval(view_hex->r1) * 16 + hexval(view_hex->r2);
  color.g = hexval(view_hex->g1) * 16 + hexval(view_hex->g2);
  color.b = hexval(view_hex->b1) * 16 + hexval(view_hex->b2);
  return color;
}

void draw_set_color_hex(draw_context_t *ctx, char *str) {
  draw_set_color(ctx, draw_rgb_hex(str));
}

void draw_github_theme(syntax_theme_t *self) {
  self->preprocessor = draw_rgb_hex("445588");
  self->identifier = draw_rgb_hex("ba2f59");
  self->comment = draw_rgb_hex("b8b6b1");
  self->keyword = draw_rgb_hex("3a81c3");
  self->builtin = draw_rgb_hex("3a81c3");
  self->string = draw_rgb_hex("2d9574");
  self->constant = draw_rgb_hex("DD1144");
}

void draw_init_syntax(syntax_theme_t *self) {
  draw_github_theme(self);
}

static __attribute__((constructor)) void __init__() {
  if (TTF_Init() != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return;
  }
  palette.primary_text = draw_rgba(0,0,0,0.87);
  palette.secondary_text = draw_rgba(0,0,0,0.17);
  palette.current_line_bg = draw_rgba(0.0, 0.0, 0, 0.06);
  palette.selection_bg = draw_rgba(0, 0, 0, 0.09);
  palette.default_bg = draw_rgba(1, 1, 1, 1);
  palette.ui_bg = draw_rgba(1, 1, 1, 1);
  palette.border = draw_rgba(0, 0, 0, 0.09);
  palette.hover = draw_rgba(0, 0, 0, 0.06);
  palette.default_font_path = "/home/vlad/job/long-glitter-c/assets/NotoSans-Regular.ttf";
  palette.monospace_font_path = "/home/vlad/job/long-glitter-c/assets/Hack-Regular.ttf";
  draw_init_font(&palette.default_font, palette.default_font_path, 16);
  draw_init_font(&palette.small_font, palette.default_font_path, 14);
  draw_init_font(&palette.fontawesome_font, "/home/vlad/job/long-glitter-c/assets/la-solid-900.ttf", 20);
  draw_init_syntax(&palette.syntax);
}

static __attribute__((destructor)) void __free__() {
  draw_free_font(&palette.default_font);
  draw_free_font(&palette.small_font);
  draw_free_font(&palette.fontawesome_font);
  TTF_Quit();
}
