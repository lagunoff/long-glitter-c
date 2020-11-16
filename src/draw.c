#include "draw.h"
#include "widget.h"

#include <stdlib.h>
#include <stdio.h>
#include <cairo.h>
#include <ctype.h>

struct hex_color {
  char r1;
  char r2;
  char g1;
  char g2;
  char b1;
  char b2;
} __attribute__((packed));

cairo_font_options_t *default_options;
cairo_matrix_t identity_matrix;

color_t draw_rgba(double r, double g, double b, double a) {
  color_t color = {r, g, b, a};
  return color;
}

void draw_init_context(widget_context_t *self, widget_context_init_t *init) {
  self->display = init->display;
  self->window = init->window;
  self->cairo = init->cairo;
  self->palette = init->palette;
  self->clip = init->clip;
  self->background = draw_rgba(1, 1, 1, 1);
  self->foreground = self->palette->primary_text;
  draw_set_font(self, &palette.default_font);
}

void draw_set_color(widget_context_t *self, color_t color) {
  self->foreground = color;
  cairo_set_source_rgba(self->cairo, color.red, color.green, color.blue, color.alpha);
}

void draw_rectangle(widget_context_t *ctx, int x, int y, int w, int h) {

}

void draw_box(widget_context_t *ctx, int x, int y, int w, int h) {
  cairo_rectangle(ctx->cairo, x, y, w, h);
  cairo_fill(ctx->cairo);
}

void draw_rect(widget_context_t *ctx, rect_t rect) {
  cairo_rectangle(ctx->cairo, rect.x, rect.y, rect.w, rect.h);
  cairo_fill(ctx->cairo);
}

void draw_set_color_rgba(widget_context_t *ctx, double r, double g, double b, double a) {
}

color_t draw_rgb_hex(char *str) {
  __auto_type view_hex = (struct hex_color *)str;
  color_t color;
  auto int hexval(char c);
  color.red = (hexval(view_hex->r1) * 16 + hexval(view_hex->r2)) / 256;
  color.green = (hexval(view_hex->g1) * 16 + hexval(view_hex->g2)) / 256;
  color.blue = (hexval(view_hex->b1) * 16 + hexval(view_hex->b2)) / 256;
  color.alpha = 1;
  return color;

  int hexval(char c) {
    return c >= '0' && c <= '9' ? c - '0' : tolower(c) - 'a' + 10;
  }
}

void draw_set_color_hex(widget_context_t *ctx, char *str) {
  draw_set_color(ctx, draw_rgb_hex(str));
}

void draw_github_theme(syntax_theme_t *self) {
  self->preprocessor = draw_rgb_hex("445588");
  self->identifier = draw_rgb_hex("ba2f59");
  self->comment = draw_rgb_hex("b8b6b1");
  self->keyword = draw_rgb_hex("3a81c3");
  self->builtin = draw_rgb_hex("3a81c3");
  self->string = draw_rgb_hex("2d9574");
  self->constant = draw_rgb_hex("dd1144");
  self->constant = draw_rgb_hex("3a81c3");
}

void draw_init_syntax(syntax_theme_t *self) {
  draw_github_theme(self);
}

void draw_set_font(widget_context_t *ctx, font_t *font) {
  cairo_set_scaled_font(ctx->cairo, font->scaled_font);
  ctx->font = font;
}

color_t draw_get_color_from_style(widget_context_t *ctx, syntax_style_t style) {
  switch (style) {
  case SYNTAX_NORMAL:       return ctx->palette->primary_text;
  case SYNTAX_PREPROCESSOR: return ctx->palette->syntax.preprocessor;
  case SYNTAX_COMMENT:      return ctx->palette->syntax.comment;
  case SYNTAX_KEYWORD:      return ctx->palette->syntax.keyword;
  case SYNTAX_BUILTIN:      return ctx->palette->syntax.builtin;
  case SYNTAX_STRING:       return ctx->palette->syntax.string;
  case SYNTAX_CONSTANT:     return ctx->palette->syntax.constant;
  case SYNTAX_IDENTIFIER:   return ctx->palette->syntax.identifier;
  case SYNTAX_TYPE:         return ctx->palette->syntax.type;
  }
}

static __attribute__((constructor)) void __init__() {
  palette.primary_text = draw_rgba(0,0,0,0.87);
  palette.secondary_text = draw_rgba(0,0,0,0.54);
  palette.current_line_bg = draw_rgba(0.0, 0.0, 0, 0.06);
  palette.selection_bg = draw_rgba(0, 0, 0, 0.09);
  palette.default_bg = draw_rgba(1, 1, 1, 1);
  palette.ui_bg = draw_rgba(1, 1, 1, 1);
  palette.border = draw_rgba(0, 0, 0, 0.09);
  palette.hover = draw_rgba(0, 0, 0, 0.06);
  draw_init_syntax(&palette.syntax);
  default_options = cairo_font_options_create();
  cairo_matrix_init_identity(&identity_matrix);

  palette.default_font.family = "Arial";
  cairo_matrix_init_scale(&palette.default_font.matrix, 16, 16);
  palette.default_font.slant = CAIRO_FONT_SLANT_NORMAL;
  palette.default_font.weight = CAIRO_FONT_WEIGHT_NORMAL;
  palette.default_font.face = cairo_toy_font_face_create(palette.default_font.family, palette.default_font.slant, palette.default_font.weight);
  palette.default_font.scaled_font = cairo_scaled_font_create(palette.default_font.face, &palette.default_font.matrix, &identity_matrix, default_options);
  cairo_scaled_font_extents(palette.default_font.scaled_font, &palette.default_font.extents);

  palette.small_font.family = "sans-serif";
  cairo_matrix_init_scale(&palette.small_font.matrix, 14, 14);
  palette.small_font.slant = CAIRO_FONT_SLANT_NORMAL;
  palette.small_font.weight = CAIRO_FONT_WEIGHT_NORMAL;
  palette.small_font.face = cairo_toy_font_face_create(palette.small_font.family, palette.small_font.slant, palette.small_font.weight);
  palette.small_font.scaled_font = cairo_scaled_font_create(palette.small_font.face, &palette.small_font.matrix, &identity_matrix, default_options);
  cairo_scaled_font_extents(palette.small_font.scaled_font, &palette.small_font.extents);

  palette.monospace_font.family = "Hack";
  cairo_matrix_init_scale(&palette.monospace_font.matrix, 16, 16);
  palette.monospace_font.slant = CAIRO_FONT_SLANT_NORMAL;
  palette.monospace_font.weight = CAIRO_FONT_WEIGHT_NORMAL;
  palette.monospace_font.face = cairo_toy_font_face_create(palette.monospace_font.family, palette.monospace_font.slant, palette.monospace_font.weight);
  palette.monospace_font.scaled_font = cairo_scaled_font_create(palette.monospace_font.face, &palette.monospace_font.matrix, &identity_matrix, default_options);
  cairo_scaled_font_extents(palette.monospace_font.scaled_font, &palette.monospace_font.extents);
}

static __attribute__((destructor)) void __free__() {
  cairo_font_options_destroy(default_options);
}
