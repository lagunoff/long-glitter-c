#pragma once
#include <cairo.h>

typedef struct {
  double r;
  double g;
  double b;
  double a;
} color_t;

void cairo_set_color(cairo_t *cr, color_t *color) {
  cairo_set_source_rgba (cr, color->r, color->g, color->b, color->a);
}
