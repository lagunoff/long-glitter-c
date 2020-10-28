#pragma once
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>

struct widget_t;

typedef struct {
  TTF_Font *font;
  int       font_size;
  int       X_width;
  int       X_height;
} draw_font_t;

typedef struct {
  SDL_Color primary_text;
  SDL_Color selection_bg;
  SDL_Color default_bg;
  SDL_Color current_line_bg;
  SDL_Color ui_bg;
  SDL_Color border;
} draw_palette_t;

typedef struct {
  SDL_Renderer  *renderer;
  SDL_Window    *window;
  SDL_Color      foreground;
  SDL_Color      background;
  draw_font_t   *font;
  draw_palette_t palette;
} draw_context_t;

SDL_Color draw_rgba(double r, double g, double b, double a);
void draw_set_color(draw_context_t *ctx, SDL_Color color);
void draw_set_color_rgba(draw_context_t *ctx, double r, double g, double b, double a);
void draw_text(draw_context_t *ctx, int x, int y, char *text);
void draw_rectangle(draw_context_t *ctx, Sint16 x, Sint16 y, Sint16 w, Sint16 h);
void draw_box(draw_context_t *ctx, Sint16 x, Sint16 y, Sint16 w, Sint16 h);
void draw_measure_text(draw_context_t *ctx, char *text, SDL_Point *size);
void draw_init_font(draw_font_t *self, int font_size);
void draw_free_font(draw_font_t *self);
void draw_init_palette(draw_palette_t *self);
void draw_init_context(draw_context_t *self, draw_font_t *font);
void draw_open_window(SDL_Rect *size_pos, Uint32 window_flags, SDL_Window **window, SDL_Renderer **renderer, struct widget_t *widget, void *model);
void draw_open_window_measure(SDL_Point *pos, Uint32 window_flags, SDL_Window **window, SDL_Renderer **renderer, struct widget_t *widget, void *model);
void draw_close_window(SDL_Window *window);
