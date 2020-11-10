#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "buff-string.h"
#include "cursor.h"
#include "buffer.h"
#include "main.h"
#include "widget.h"

int main(int argc, char **argv) {
  buffer_t buf;
  __auto_type ctx = &buf.ctx;

  char *path = argc < 2 ? "/home/vlad/job/long-glitter-c/tmp/xola.c" : argv[1];

  if (SDL_Init(0) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((ctx->window = SDL_CreateWindow("Long Glitter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE)) == NULL) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((ctx->renderer = SDL_CreateRenderer(ctx->window, -1, 0)) == NULL) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  buffer_init(&buf, path, 14);
  draw_set_color(ctx, ctx->background);
  widget_window_set(ctx->window, (widget_t)&buffer_dispatch, (void *)&buf);

  SDL_Event e;
  SDL_StartTextInput();
  int keydowns = 0;

  SDL_Window *event_window = NULL;
  SDL_Renderer *event_renderer = NULL;
  widget_t root_widget = NULL;
  void *root_widget_data = NULL;

  void loop(widget_msg_t *msg) {
    bool do_render = msg->tag == MSG_VIEW;
    if (do_render) {
      event_renderer = event_window ? SDL_GetRenderer(event_window) : ctx->renderer;
      SDL_RenderSetViewport(event_renderer, NULL);
    }
    event_window = NULL;
    root_widget(root_widget_data, msg, (yield_t)&loop);
    if (do_render) SDL_RenderPresent(event_renderer);
  }

  while (SDL_WaitEvent(&e) == 1) {
    if (e.type == SDL_QUIT) break;
    event_window = widget_get_event_window(&e);
    if (event_window == NULL) continue;
    widget_window_get(event_window, &root_widget, &root_widget_data);
    if (!root_widget || !root_widget_data) continue;

    widget_msg_t msg[4];
    msg[0] = msg_sdl_event(&e);
    loop(msg);
    continue;
  }

  buffer_free(&buf);
  SDL_StopTextInput();
  SDL_DestroyRenderer(ctx->renderer);
  SDL_DestroyWindow(ctx->window);

  return EXIT_SUCCESS;
}
