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
#include "dlist.h"
#include "graphics.h"
#include "main-window.h"

int main(int argc, char **argv) {
  int width = 800, height = 800;
  XVisualInfo vinfo;
  XSetWindowAttributes attr;
  widget_window_head_t window_widget = {NULL, NULL};
  __auto_type root_node = new(((widget_window_node_t){NULL,NULL,0,NULL,NULL}));
  __auto_type ctx = &root_node->ctx;
  __auto_type event_mask = ButtonPressMask|KeyPressMask|ExposureMask|StructureNotifyMask|PointerMotionMask|KeyPressMask|PropertyChangeMask|ButtonPress;

  ctx->display = XOpenDisplay(NULL); exit_if_not(ctx->display);
  XMatchVisualInfo(ctx->display, DefaultScreen(ctx->display), 32, TrueColor, &vinfo);
  attr.colormap = XCreateColormap(ctx->display, DefaultRootWindow(ctx->display), vinfo.visual, AllocNone);
  attr.border_pixel = 0;
  attr.background_pixel = ULONG_MAX;
  ctx->window = XCreateWindow(ctx->display, DefaultRootWindow(ctx->display), 0, 0, width, height, 0, vinfo.depth, 0, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attr);
  root_node->window = ctx->window;
  __auto_type screen = XDefaultScreen(ctx->display);
  root_node->surface = cairo_xlib_surface_create(ctx->display, ctx->window, vinfo.visual, width, height);
  XSelectInput(ctx->display, ctx->window, event_mask);
  XMapWindow(ctx->display, ctx->window);
  gx_init(ctx->display);

  XIM xim = XOpenIM(ctx->display, 0, 0, 0);
  ctx->xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, NULL);

  ctx->palette = &palette;
  ctx->cairo = cairo_create(root_node->surface);
  ctx->font = &palette.default_font;

  main_window_t buffer;
  main_window_init(&buffer, ctx, argc, argv);
  main_window_msg_t layout = {.tag = Widget_Layout};
  root_node->widget = coerce_widget(&buffer.widget);
  XEvent x_event;
  char keybuf[8];
  KeySym key;
  widget_window_node_t *current_widget = NULL;

  root_node->msg = (void**)&root_node->msg_head;
  dlist_insert_before((dlist_head_t *)&window_widget, (dlist_node_t *)root_node, NULL);

  void loop(void *user_msg) {// XEvent
    if (!user_msg) {
      widget_window_node_t *iter = window_widget.first;
      for(;iter; iter = iter->next) {
        // FIXME: Will x_event.xany.window work with clipboard events?
        if (iter->window == x_event.xany.window) {
          current_widget = iter;
          break;
        }
      }
      if (!current_widget) return;

      switch (x_event.type) {
      case Expose: {
        gx_set_color(&current_widget->ctx, gx_rgba(1,1,1,0));
        cairo_paint(current_widget->ctx.cairo);
        break;
      }
      case ConfigureNotify: {
        if (x_event.xconfigure.width != current_widget->widget->basic.clip.w || x_event.xconfigure.height != current_widget->widget->basic.clip.h) {
          current_widget->widget->basic.clip.w = x_event.xconfigure.width;
          current_widget->widget->basic.clip.h = x_event.xconfigure.height;
          cairo_xlib_surface_set_size(current_widget->surface, x_event.xconfigure.width, x_event.xconfigure.height);
          return loop(&msg_layout);
        }
        return;
      }}
    } else {
      __auto_type msg = (widget_msg_t *)user_msg;
      switch (msg->tag) {
      case Widget_NewWindow: {
        __auto_type new_node = new(((widget_window_node_t){
          NULL, NULL, msg->new_window.window, msg->new_window.widget, msg->new_window.msg_head, msg->new_window.msg, .ctx=*ctx,
        }));
        new_node->surface = cairo_xlib_surface_create(ctx->display, msg->new_window.window, vinfo.visual, msg->new_window.width, msg->new_window.height);
        new_node->ctx.window = msg->new_window.window;
        new_node->ctx.cairo = cairo_create(new_node->surface);
        new_node->widget->basic.ctx = &new_node->ctx;
        *new_node->msg = &msg_layout;
        buffer.widget.dispatch(&buffer, new_node->msg_head, &loop);
        dlist_insert_before((dlist_head_t *)&window_widget, (dlist_node_t *)new_node, NULL);
        XSelectInput(ctx->display, new_node->window, event_mask);
        XMapWindow(ctx->display, new_node->window);
        return;
      }
      case Widget_CloseWindow: {
        dlist_filter(
          (dlist_head_t *)&window_widget,
          lambda(void _(dlist_node_t *dnode){
            __auto_type node = (widget_window_node_t *)dnode;
            XUnmapWindow(ctx->display, node->window);
            node->widget->basic.ctx = NULL;
            cairo_surface_destroy(node->surface);
            cairo_destroy(node->ctx.cairo);
            free(node);
          }),
          lambda(bool _(dlist_node_t *dnode){
            __auto_type node = (widget_window_node_t *)dnode;
            return node->window != msg->close_window;
          })
        );
      }}
    }

    if (!current_widget) return;
    *current_widget->msg = user_msg ? : &x_event;
    buffer.widget.dispatch(&buffer, current_widget->msg_head, &loop);
    if (!user_msg) {
      current_widget = NULL;
    }
  }

  buffer.widget.clip.x = 0; buffer.widget.clip.y = 0;
  buffer.widget.clip.w = width; buffer.widget.clip.h = height;
  dispatch_to(&loop, &buffer, &layout);
  while (1) {
    XNextEvent(ctx->display, &x_event);
    loop(NULL);
  }

  gx_free(ctx->display);
  /* cairo_destroy(ctx.cairo); */
  /* cairo_surface_destroy(surface); */
  XCloseDisplay(ctx->display);
  return EXIT_SUCCESS;
}
