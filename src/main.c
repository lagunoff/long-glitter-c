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
#include "graphics.h"
#include "main-window.h"

int main(int argc, char **argv) {
  __auto_type path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];
  int width = 800, height = 800;
  XVisualInfo vinfo;
  XSetWindowAttributes attr;
  widget_context_t ctx;

  ctx.display = XOpenDisplay(NULL); exit_if_not(ctx.display);
  XMatchVisualInfo(ctx.display, DefaultScreen(ctx.display), 32, TrueColor, &vinfo);
  attr.colormap = XCreateColormap(ctx.display, DefaultRootWindow(ctx.display), vinfo.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = ULONG_MAX;
  ctx.window = XCreateWindow(ctx.display, DefaultRootWindow(ctx.display), 0, 0, width, height, 0, vinfo.depth, 0, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
  __auto_type screen = XDefaultScreen(ctx.display);
  __auto_type surface = cairo_xlib_surface_create(ctx.display, ctx.window, vinfo.visual, width, height);
  XSelectInput(ctx.display, ctx.window, ButtonPressMask|KeyPressMask|ExposureMask|StructureNotifyMask|PointerMotionMask|KeyPressMask|PropertyChangeMask|ButtonPress);
  XMapWindow(ctx.display, ctx.window);
  gx_init(ctx.display);

  XIM xim = XOpenIM(ctx.display, 0, 0, 0);
  ctx.xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);

  ctx.palette = &palette;
  ctx.cairo = cairo_create(surface);
  ctx.font = &palette.default_font;

  main_window_t buffer;
  main_window_init(&buffer, &ctx, argc, argv);
  main_window_msg_t layout = {.tag = Widget_Layout};

  XEvent x_event;
  char keybuf[8];
  KeySym key;

  void loop(void *user_msg) {
    if (!user_msg) {
      switch (x_event.type) {
      case ConfigureNotify: {
        if (x_event.xconfigure.width != buffer.widget.clip.w || x_event.xconfigure.height != buffer.widget.clip.h) {
          buffer.widget.clip.w = x_event.xconfigure.width;
          buffer.widget.clip.h = x_event.xconfigure.height;
          cairo_xlib_surface_set_size(surface, buffer.widget.clip.w, buffer.widget.clip.h);
          loop(&msg_layout);
        }
        return;
      }
      case Expose: {
        gx_set_color(buffer.widget.ctx, buffer.widget.ctx->palette->default_bg);
        cairo_paint(buffer.widget.ctx->cairo);
        break;
      }}
    }
    main_window_dispatch(&buffer, user_msg ? user_msg : &x_event, &loop);
  }

  buffer.widget.clip.x = 0; buffer.widget.clip.y = 0;
  buffer.widget.clip.w = width; buffer.widget.clip.h = height;
  main_window_dispatch(&buffer, &layout, &loop);
  while (1) {
    XNextEvent(ctx.display, &x_event);
    loop(NULL);
  }

  gx_free(ctx.display);
  cairo_destroy(ctx.cairo);
  cairo_surface_destroy(surface);
  XCloseDisplay(ctx.display);
  return EXIT_SUCCESS;
}
