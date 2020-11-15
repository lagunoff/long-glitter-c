#pragma once
#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

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
  XRenderColor preprocessor;
  XRenderColor comment;
  XRenderColor keyword;
  XRenderColor builtin;
  XRenderColor string;
  XRenderColor constant;
  XRenderColor identifier;
  XRenderColor type;
} syntax_theme_t;

typedef struct {
  XRenderColor primary_text;
  XRenderColor secondary_text;
  XRenderColor selection_bg;
  XRenderColor default_bg;
  XRenderColor current_line_bg;
  XRenderColor ui_bg;
  XRenderColor border;
  XRenderColor hover;
  XftFont     *default_font;
  XftFont     *small_font;
  XftFont     *monospace_font;
  XftFont     *fontawesome_font;
  syntax_theme_t syntax;
} draw_palette_t;

typedef struct {
  Display        *display;
  Window          window;
  XftDraw        *xftdraw;
  Picture         picture;
  GC              gc;
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
  XftDraw        *xftdraw;
  Picture         picture;
  GC              gc;
  draw_palette_t *palette;
  // rw fields
  rect_t          clip;
  XftFont        *font;
  XRenderColor    foreground;
  XftColor        foreground_xft;
  XRenderColor    background;
  XftColor        background_xft;
} draw_context_t;

draw_palette_t palette;

void draw_init_context(draw_context_t *self, draw_context_init_t *data);
XRenderColor draw_rgba(double r, double g, double b, double a);
XRenderColor draw_rgb_hex(char *str);
void draw_set_color(draw_context_t *ctx, XRenderColor color);
void draw_set_color_rgba(draw_context_t *ctx, double r, double g, double b, double a);
void draw_set_color_hex(draw_context_t *ctx, char *str);
void draw_rectangle(draw_context_t *ctx, int x, int y, int w, int h);
void draw_box(draw_context_t *ctx, int x, int y, int w, int h);
void draw_rect(draw_context_t *ctx, rect_t rect);
void draw_init_font(XftFont *self, char *path, int font_size);
void draw_free_font(XftFont *self);
void draw_free_font(XftFont *self);
XRenderColor draw_get_color_from_style(draw_context_t *ctx, syntax_style_t style);
void draw_init(Display *dpy);
void draw_free(Display *dpy);

__inline__ __attribute__((always_inline)) void
draw_text(draw_context_t *ctx, int x, int y, char *text, int len) {
  XftDrawString8(ctx->xftdraw, &ctx->foreground_xft, ctx->font, x, y, text, len);
}

__inline__ __attribute__((always_inline)) void
draw_measure_text(draw_context_t *ctx, char *text, int len, XGlyphInfo *extents) {
  XftTextExtents8(ctx->display, ctx->font, text, len, extents);
}
