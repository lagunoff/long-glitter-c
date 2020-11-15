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
  int width = 900, height = 500;
  draw_context_ro_t app_ctx;
  __auto_type path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];

  XSetWindowAttributes win_attr;
  app_ctx.display = XOpenDisplay(NULL); exit_if_not(app_ctx.display);


  XVisualInfo vinfo;
  XMatchVisualInfo(app_ctx.display, DefaultScreen(app_ctx.display), 32, TrueColor, &vinfo);

  XSetWindowAttributes attr;
  attr.colormap = XCreateColormap(app_ctx.display, DefaultRootWindow(app_ctx.display), vinfo.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = ULONG_MAX;

  app_ctx.window = XCreateWindow(app_ctx.display, DefaultRootWindow(app_ctx.display), 0, 0, width, height, 0, vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);

  draw_init(app_ctx.display);
  __auto_type screen = XDefaultScreen(app_ctx.display);
  __auto_type surface = cairo_xlib_surface_create(app_ctx.display, app_ctx.window, vinfo.visual, width, height);
  XSelectInput(app_ctx.display, app_ctx.window, ButtonPressMask|KeyPressMask|ExposureMask|ResizeRedirectMask);
  XMapWindow(app_ctx.display, app_ctx.window);
  app_ctx.palette = &palette;
  app_ctx.cairo = cairo_create(surface);
  draw_context_init_t init_ctx = {.ro = &app_ctx, .clip = {0,0,width,height}};

  buffer_t buffer;
  buffer_init(&buffer, &init_ctx , path);

  XEvent e;
  char keybuf[8];
  KeySym key;

  void loop(void *msg) {
    buffer_dispatch(&buffer, msg ? msg : &e, (yield_t)&loop);
  }

  while (1) {
    XNextEvent(app_ctx.display, &e);
    loop(NULL);
  }

  draw_free(app_ctx.display);
  XRenderFreePicture(app_ctx.display, app_ctx.picture);
  XCloseDisplay(app_ctx.display);
  return EXIT_SUCCESS;
}
