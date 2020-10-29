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

  char *path = argc < 2 ? "/home/vlad/job/long-glitter-c/src/buff-string.c" : argv[1];

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((ctx->window = SDL_CreateWindow("Long Glitter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE)) == NULL) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  buffer_init(&buf, path, 14);
  draw_set_color(ctx, ctx->background);
  widget_window_set(ctx->window, &buffer_widget, (void *)&buf);

  SDL_Event e;
  SDL_StartTextInput();
  int keydowns = 0;

  SDL_Window *event_window = NULL;
  SDL_Renderer *event_renderer = NULL;
  widget_t *root_widget = NULL;
  void *root_widget_data = NULL;
  bool do_render = false;
  bool do_update = false;
  while (SDL_WaitEvent(&e) == 1) {
    do_render = false;
    do_update = false;

    if (e.type == SDL_QUIT) {
      break;
    }

    if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.scancode == SDL_SCANCODE_RETURN && keydowns==0) {
        continue;
      }
      keydowns = 1;
      do_update = true;
      event_window = SDL_GetWindowFromID(e.key.windowID);
      goto end_iteration;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
      do_update = true;
      event_window = SDL_GetWindowFromID(e.button.windowID);
      goto end_iteration;
    }

    if (e.type == SDL_MOUSEMOTION) {
      do_update = true;
      event_window = SDL_GetWindowFromID(e.motion.windowID);
      goto end_iteration;
    }

    if (e.type == SDL_WINDOWEVENT) {
      event_window = SDL_GetWindowFromID(e.window.windowID);
      do_update = true;
      goto end_iteration;
    }
    if (e.type == SDL_TEXTINPUT) {
      event_window = SDL_GetWindowFromID(e.text.windowID);
      do_update = true;
      goto end_iteration;
    }
    if (e.type == SDL_MOUSEWHEEL) {
      event_window = SDL_GetWindowFromID(e.wheel.windowID);
      do_update = true;
      goto end_iteration;
    }

  end_iteration:
    if (event_window == NULL) continue;
    event_renderer = SDL_GetRenderer(event_window);
    widget_window_get(event_window, &root_widget, &root_widget_data);
    if (!root_widget || !root_widget_data) continue;
    if (do_update && root_widget->update) do_render = root_widget->update(root_widget_data, &e);
    if (!do_render) continue;
    if (event_renderer == NULL) continue;
    SDL_RenderSetViewport(event_renderer, NULL);
    root_widget->view(root_widget_data);
    SDL_RenderPresent(event_renderer);
    event_window = NULL;
    continue;
  }

  buffer_destroy(&buf);
  SDL_StopTextInput();
  SDL_DestroyRenderer(ctx->renderer);
  SDL_DestroyWindow(ctx->window);

  return EXIT_SUCCESS;
}
