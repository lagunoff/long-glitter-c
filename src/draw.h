#pragma once
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <cairo.h>

#include "utils.h"

struct widget_t;

typedef enum {
  SYNTAX_NORMAL,
  SYNTAX_PREPROCESSOR,
  SYNTAX_COMMENT,
  SYNTAX_KEYWORD,
  SYNTAX_BUILTIN,
  SYNTAX_STRING,
  SYNTAX_CONSTANT,
  SYNTAX_IDENTIFIER,
  SYNTAX_TYPE,
} syntax_style_t;

typedef struct {
  double red, green, blue, alpha;
} color_t;

typedef struct {
  // Arguments
  char                *family;
  cairo_font_slant_t   slant;
  cairo_font_weight_t  weight;
  cairo_matrix_t       matrix;
  // Cached data
  cairo_font_extents_t extents;
  cairo_font_face_t   *face;
  cairo_scaled_font_t *scaled_font;
} font_t;

typedef struct {
  color_t preprocessor;
  color_t comment;
  color_t keyword;
  color_t builtin;
  color_t string;
  color_t constant;
  color_t identifier;
  color_t type;
} syntax_theme_t;

typedef struct {
  color_t primary_text;
  color_t secondary_text;
  color_t selection_bg;
  color_t default_bg;
  color_t current_line_bg;
  color_t ui_bg;
  color_t border;
  color_t hover;
  font_t       default_font;
  font_t       small_font;
  font_t       monospace_font;
  font_t       fontawesome_font;
  syntax_theme_t syntax;
} draw_palette_t;

typedef struct {
  Display        *display;
  Window          window;
  Picture         picture;
  cairo_t        *cairo;
  draw_palette_t *palette;
} draw_context_ro_t;

typedef struct {
  draw_context_ro_t *ro;
  rect_t             clip;
} draw_context_init_t;

typedef struct {
  // ro fields
  Display        *display;
  Window          window;
  Picture         picture;
  cairo_t        *cairo;
  draw_palette_t *palette;
  // rw fields
  rect_t          clip;
  font_t          font;
  color_t    foreground;
  color_t    background;
} draw_context_t;

draw_palette_t palette;

void draw_init_context(draw_context_t *self, draw_context_init_t *data);
color_t draw_rgba(double r, double g, double b, double a);
color_t draw_rgb_hex(char *str);
void draw_set_color(draw_context_t *ctx, color_t color);
void draw_set_color_rgba(draw_context_t *ctx, double r, double g, double b, double a);
void draw_set_color_hex(draw_context_t *ctx, char *str);
void draw_rectangle(draw_context_t *ctx, int x, int y, int w, int h);
void draw_box(draw_context_t *ctx, int x, int y, int w, int h);
void draw_rect(draw_context_t *ctx, rect_t rect);
void draw_set_font(draw_context_t *ctx, font_t *font);
color_t draw_get_color_from_style(draw_context_t *ctx, syntax_style_t style);
void draw_init(Display *dpy);
void draw_free(Display *dpy);

__inline__ __attribute__((always_inline)) void
draw_text(draw_context_t *ctx, int x, int y, const char *text, int len) {
  cairo_move_to(ctx->cairo, x, y);
  cairo_show_text(ctx->cairo, text);
}

__inline__ __attribute__((always_inline)) void
draw_measure_text(draw_context_t *ctx, char *text, int len, cairo_text_extents_t *extents) {
  cairo_text_extents(ctx->cairo, text, extents);
}
