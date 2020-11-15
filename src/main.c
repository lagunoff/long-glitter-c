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
#include <X11/Xft/Xft.h>

#include "utils.h"
#include "draw.h"
#include "buffer.h"

int main(int argc, char **argv) {
  int width = 900, height = 500;
  draw_context_ro_t app_ctx;
  __auto_type path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];
  app_ctx.display = XOpenDisplay(NULL); exit_if_not(app_ctx.display);
  XSetWindowAttributes win_attr;
  __auto_type screen = DefaultScreen(app_ctx.display);
  app_ctx.window = XCreateWindow(app_ctx.display, DefaultRootWindow(app_ctx.display), 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, 0, &win_attr); exit_if_not(app_ctx.window);
  XSelectInput(app_ctx.display, app_ctx.window, ButtonPressMask|KeyPressMask|ExposureMask);
  XMapWindow(app_ctx.display, app_ctx.window);
  draw_init(app_ctx.display);
  app_ctx.xftdraw = XftDrawCreate(app_ctx.display,app_ctx.window,DefaultVisual(app_ctx.display,0),DefaultColormap(app_ctx.display,0)); exit_if_not(app_ctx.xftdraw);
  app_ctx.palette = &palette;
  XWindowAttributes attr;
  XGetWindowAttributes(app_ctx.display, app_ctx.window, &attr);
  __auto_type picture_format = XRenderFindVisualFormat(app_ctx.display, attr.visual);
  XRenderPictureAttributes pa;
  pa.subwindow_mode = IncludeInferiors;
  pa.component_alpha = True;
  app_ctx.picture = XRenderCreatePicture(app_ctx.display, app_ctx.window, picture_format, CPSubwindowMode | CPComponentAlpha, &pa);
  XGCValues values;
  values.foreground = WhitePixel(app_ctx.display, screen);
  values.background = WhitePixel(app_ctx.display, screen);
  app_ctx.gc = XCreateGC(app_ctx.display, app_ctx.window, (GCForeground | GCBackground), &values);

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
