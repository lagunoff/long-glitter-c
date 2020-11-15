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


XRenderColor draw_rgba(double r, double g, double b, double a) {
  XRenderColor color = {r*65535, g*65535, b*65535, a*65535};
  return color;
}

void draw_init_context(draw_context_t *self, draw_context_init_t *data) {
  self->display = data->ro->display;
  self->window = data->ro->window;
  self->xftdraw = data->ro->xftdraw;
  self->picture = data->ro->picture;
  self->palette = data->ro->palette;
  self->font = palette.default_font;
  self->clip = data->clip;
  self->background = draw_rgba(1, 1, 1, 1);
  self->foreground = self->palette->primary_text;
  XftColorAllocValue(self->display, DefaultVisual(self->display,0),DefaultColormap(self->display,0), &self->foreground, &self->foreground_xft);
}

void draw_set_color(draw_context_t *self, XRenderColor color) {
  self->foreground = color;
  XftColorFree(self->display, DefaultVisual(self->display,0),DefaultColormap(self->display,0), &self->foreground_xft);
  XftColorAllocValue(self->display, DefaultVisual(self->display,0),DefaultColormap(self->display,0), &self->foreground, &self->foreground_xft);
}

void draw_rectangle(draw_context_t *ctx, int x, int y, int w, int h) {

}

void draw_box(draw_context_t *ctx, int x, int y, int w, int h) {
  XftDrawRect(ctx->xftdraw, &ctx->foreground_xft, x, y, w, h);
  //  XRenderFillRectangle(ctx->display, ctx->window, ctx->picture, &ctx->foreground, x, y, w, h);
}

void draw_rect(draw_context_t *ctx, rect_t rect) {
  XftDrawRect(ctx->xftdraw, &ctx->foreground_xft, rect.x, rect.y, rect.w, rect.h);
  //  XDrawRectangle(ctx->display,ctx->window, ctx->display,ctx->window);
  //  XRenderFillRectangle(ctx->display,ctx->window,ctx->picture, &ctx->foreground, rect.x, rect.y, rect.w, rect.h);
}

void draw_set_color_rgba(draw_context_t *ctx, double r, double g, double b, double a) {
}

XRenderColor draw_rgb_hex(char *str) {
  __auto_type view_hex = (struct hex_color *)str;
  XRenderColor color;
  auto int hexval(char c);
  color.red = (hexval(view_hex->r1) * 16 + hexval(view_hex->r2)) * 256;
  color.green = (hexval(view_hex->g1) * 16 + hexval(view_hex->g2)) * 256;
  color.blue = (hexval(view_hex->b1) * 16 + hexval(view_hex->b2)) * 256;
  color.alpha = 65535;
  return color;

  int hexval(char c) {
    return c >= '0' && c <= '9' ? c - '0' : tolower(c) - 'a' + 10;
  }
}

void draw_set_color_hex(draw_context_t *ctx, char *str) {
  draw_set_color(ctx, draw_rgb_hex(str));
}

void draw_github_theme(syntax_theme_t *self) {
  self->preprocessor = draw_rgb_hex("445588");
  //  XRenderParseColor(self->preprocessor, "#445588");
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

XRenderColor draw_get_color_from_style(draw_context_t *ctx, syntax_style_t style) {
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

void draw_init(Display *dpy) {
  palette.default_font = XftFontOpen(dpy, DefaultScreen(dpy), XFT_FAMILY, XftTypeString, "sans-serif", XFT_SIZE, XftTypeDouble, 16.0, NULL);
  palette.small_font = XftFontOpen(dpy, DefaultScreen(dpy), XFT_FAMILY, XftTypeString, "sans-serif", XFT_SIZE, XftTypeDouble, 14.0, NULL);
  palette.monospace_font = XftFontOpen(dpy, DefaultScreen(dpy), XFT_FAMILY, XftTypeString, "monospace", XFT_SIZE, XftTypeDouble, 16.0, NULL);
}

void draw_free(Display *dpy) {
  XftFontClose(dpy, palette.default_font);
  XftFontClose(dpy, palette.small_font);
  XftFontClose(dpy, palette.monospace_font);
}

static __attribute__((constructor)) void __init__() {
  palette.primary_text = draw_rgba(0,.99,0,0.2);
  palette.secondary_text = draw_rgba(0,0,0,0.17);
  palette.current_line_bg = draw_rgba(0.8, 0.0, 0, 0.06);
  palette.selection_bg = draw_rgba(0, 0, 0, 0.09);
  palette.default_bg = draw_rgba(1, 1, 1, 1);
  palette.ui_bg = draw_rgba(1, 1, 1, 1);
  palette.border = draw_rgba(0, 0, 0, 0.09);
  palette.hover = draw_rgba(0, 0, 0, 0.06);
  draw_init_syntax(&palette.syntax);
}
