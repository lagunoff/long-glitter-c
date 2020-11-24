#include "graphics.h"
#include "widget.h"

#include <stdlib.h>
#include <stdio.h>
#include <cairo.h>
#include <ctype.h>
#include <X11/Xcursor/Xcursor.h>

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

color_t gx_rgba(double r, double g, double b, double a) {
  color_t color = {r, g, b, a};
  return color;
}

void gx_init_context(widget_context_t *self, widget_context_init_t *init) {
  self->display = init->display;
  self->window = init->window;
  self->cairo = init->cairo;
  self->palette = init->palette;
  self->clip = init->clip;
  self->xic = init->xic;
  self->background = gx_rgba(1, 1, 1, 1);
  self->foreground = self->palette->primary_text;
  gx_set_font(self, &palette.default_font);
}

void gx_set_color(widget_context_t *self, color_t color) {
  self->foreground = color;
  cairo_set_source_rgba(self->cairo, color.red, color.green, color.blue, color.alpha);
}

void gx_rectangle(widget_context_t *ctx, int x, int y, int w, int h) {
  cairo_rectangle(ctx->cairo, x, y, w, h);
  cairo_fill(ctx->cairo);
}

void gx_box(widget_context_t *ctx, int x, int y, int w, int h) {
  cairo_rectangle(ctx->cairo, x, y, w, h);
  cairo_fill(ctx->cairo);
}

void gx_rect(widget_context_t *ctx, rect_t rect) {
  cairo_rectangle(ctx->cairo, rect.x, rect.y, rect.w, rect.h);
  cairo_fill(ctx->cairo);
}

void gx_set_color_rgba(widget_context_t *self, double r, double g, double b, double a) {
  self->foreground = gx_rgba(r, g, b, a);
  cairo_set_source_rgba(self->cairo, r, g, b, a);
}

color_t gx_rgb_hex(char *str) {
  auto int hexval(char c);
  __auto_type view_hex = (struct hex_color *)str;
  double red = hexval(view_hex->r1) * 16 + hexval(view_hex->r2);
  double green = hexval(view_hex->g1) * 16 + hexval(view_hex->g2);
  double blue = hexval(view_hex->b1) * 16 + hexval(view_hex->b2);
  color_t color = {red / 255.0, green / 255.0, blue / 255.0, 1};
  return color;

  int hexval(char c) {
    return c >= '0' && c <= '9' ? c - '0' : tolower(c) - 'a' + 10;
  }
}

void gx_set_color_hex(widget_context_t *ctx, char *str) {
  gx_set_color(ctx, gx_rgb_hex(str));
}

void gx_github_theme(syntax_theme_t *self) {
  self->preprocessor = gx_rgb_hex("445588");
  self->identifier = gx_rgb_hex("ba2f59");
  self->comment = gx_rgb_hex("b8b6b1");
  self->keyword = gx_rgb_hex("3a81c3");
  self->builtin = gx_rgb_hex("3a81c3");
  self->string = gx_rgb_hex("2d9574");
  self->constant = gx_rgb_hex("dd1144");
  self->type = gx_rgb_hex("3a81c3");
}

void gx_init_syntax(syntax_theme_t *self) {
  gx_github_theme(self);
}

void gx_set_font(widget_context_t *ctx, font_t *font) {
  cairo_set_scaled_font(ctx->cairo, font->scaled_font);
  ctx->font = font;
}

color_t gx_get_color_from_style(widget_context_t *ctx, syntax_style_t style) {
  switch (style) {
  case Syntax_Normal:       return ctx->palette->primary_text;
  case Syntax_Preprocessor: return ctx->palette->syntax.preprocessor;
  case Syntax_Comment:      return ctx->palette->syntax.comment;
  case Syntax_Keyword:      return ctx->palette->syntax.keyword;
  case Syntax_Builtin:      return ctx->palette->syntax.builtin;
  case Syntax_String:       return ctx->palette->syntax.string;
  case Syntax_Constant:     return ctx->palette->syntax.constant;
  case Syntax_Identifier:   return ctx->palette->syntax.identifier;
  case Syntax_Type:         return ctx->palette->syntax.type;
  }
}

void gx_init(Display *display) {
  palette.arrow = XcursorLibraryLoadCursor(display, "arrow");
  palette.xterm = XcursorLibraryLoadCursor(display, "xterm");
  palette.hand1 = XcursorLibraryLoadCursor(display, "hand1");
}

void gx_free(Display *display) {
}

static __attribute__((constructor)) void __init__() {
  palette.primary_text = gx_rgba(0,0,0,0.87);
  palette.secondary_text = gx_rgba(0,0,0,0.54);
  palette.current_line_bg = gx_rgb_hex("e4f6d4");
  palette.selection_bg = gx_rgb_hex("dfe2e7");
  palette.default_bg = gx_rgba(0.97, 0.97, 0.97, 1);
  palette.ui_bg = gx_rgba(1, 1, 1, 1);
  palette.border = gx_rgba(0, 0, 0, 0.09);
  palette.hover = gx_rgba(0, 0, 0, 0.06);
  gx_init_syntax(&palette.syntax);
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
