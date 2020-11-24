#pragma once
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <cairo.h>

#include "utils.h"

struct widget_t;

typedef enum {
  Syntax_Normal,
  Syntax_Preprocessor,
  Syntax_Comment,
  Syntax_Keyword,
  Syntax_Builtin,
  Syntax_String,
  Syntax_Constant,
  Syntax_Identifier,
  Syntax_Type,
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
  // Cached results
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
  font_t  default_font;
  font_t  small_font;
  font_t  monospace_font;
  font_t  fontawesome_font;
  Cursor  arrow;
  Cursor  xterm;
  Cursor  hand1;
  syntax_theme_t syntax;
} palette_t;

typedef struct {
  Display   *display;
  Window     window;
  cairo_t   *cairo;
  palette_t *palette;
  XIC        xic;
  rect_t     clip;
} widget_context_init_t;

typedef struct {
  // ro fields
  Display   *display;
  Window     window;
  cairo_t   *cairo;
  palette_t *palette;
  XIC        xic;
  // rw fields
  rect_t     clip;
  font_t    *font;
  color_t    foreground;
  color_t    background;
} widget_context_t;

palette_t palette;

void gx_init_context(widget_context_t *self, widget_context_init_t *data);
color_t gx_rgba(double r, double g, double b, double a);
color_t gx_rgb_hex(char *str);
void gx_set_color(widget_context_t *ctx, color_t color);
void gx_set_color_rgba(widget_context_t *ctx, double r, double g, double b, double a);
void gx_set_color_hex(widget_context_t *ctx, char *str);
void gx_rectangle(widget_context_t *ctx, int x, int y, int w, int h);
void gx_box(widget_context_t *ctx, int x, int y, int w, int h);
void gx_rect(widget_context_t *ctx, rect_t rect);
void gx_set_font(widget_context_t *ctx, font_t *font);
color_t gx_get_color_from_style(widget_context_t *ctx, syntax_style_t style);
void gx_init(Display *display);
void gx_free(Display *display);

inline_always void
gx_text(widget_context_t *ctx, int x, int y, const char *text) {
  cairo_move_to(ctx->cairo, x, y);
  cairo_show_text(ctx->cairo, text);
}

inline_always void
gx_measure_text(widget_context_t *ctx, char *text, cairo_text_extents_t *extents) {
  cairo_text_extents(ctx->cairo, text, extents);
}

inline_always void
gx_line(widget_context_t *ctx, int x1, int y1, int x2, int y2) {
  cairo_move_to(ctx->cairo, x1, y1);
  cairo_line_to(ctx->cairo, x2, y2);
  cairo_stroke(ctx->cairo);
}