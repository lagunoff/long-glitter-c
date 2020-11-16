#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "utils.h"
#include "draw.h"
#include "buffer.h"

int main(int argc, char **argv) {
  __auto_type path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];
  int width = 900, height = 500;
  XVisualInfo vinfo;
  XSetWindowAttributes attr;
  widget_context_init_t ctx;

  ctx.display = XOpenDisplay(NULL); exit_if_not(ctx.display);
  XMatchVisualInfo(ctx.display, DefaultScreen(ctx.display), 32, TrueColor, &vinfo);
  attr.colormap = XCreateColormap(ctx.display, DefaultRootWindow(ctx.display), vinfo.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = ULONG_MAX;
  ctx.window = XCreateWindow(ctx.display, DefaultRootWindow(ctx.display), 0, 0, width, height, 0, vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
  __auto_type screen = XDefaultScreen(ctx.display);
  __auto_type surface = cairo_xlib_surface_create(ctx.display, ctx.window, vinfo.visual, width, height);
  XSelectInput(ctx.display, ctx.window, ButtonPressMask|KeyPressMask|ExposureMask|StructureNotifyMask);
  XMapWindow(ctx.display, ctx.window);
  ctx.palette = &palette;
  ctx.cairo = cairo_create(surface);
  ctx.clip.x = 0; ctx.clip.y = 0;
  ctx.clip.w = width; ctx.clip.h = height;

  buffer_t buffer;
  buffer_init(&buffer, &ctx , path);

  XEvent x_event;
  char keybuf[8];
  KeySym key;

  void loop(void *user_msg) {
    if (!user_msg) {
      switch (x_event.type) {
      case ConfigureNotify: {
        if (x_event.xconfigure.width != buffer.ctx.clip.w || x_event.xconfigure.height != buffer.ctx.clip.h) {
          buffer.ctx.clip.w = x_event.xconfigure.width;
          buffer.ctx.clip.h = x_event.xconfigure.height;
          cairo_xlib_surface_set_size(surface, buffer.ctx.clip.w, buffer.ctx.clip.h);
          loop(&msg_layout);
        }
        return;
      }
      case Expose: {
        draw_set_color(&buffer.ctx, buffer.ctx.background);
        cairo_paint(buffer.ctx.cairo);
        break;
      }}
    }
    buffer_dispatch(&buffer, user_msg ? user_msg : &x_event, (yield_t)&loop);
  }

  while (1) {
    XNextEvent(ctx.display, &x_event);
    loop(NULL);
  }

  cairo_destroy(ctx.cairo);
  cairo_surface_destroy(surface);
  XCloseDisplay(ctx.display);
  return EXIT_SUCCESS;
}
