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


int main(int argc, char **argv) {
  SDL_Renderer *renderer;
  SDL_Window *window;
  char *path = argc < 2 ? "/home/vlad/job/long-glitter-c/test/01.lisp" : argv[1];

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((window = SDL_CreateWindow("Long Glitter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE)) == NULL) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if ((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if (TTF_Init() != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Point size = {WINDOW_WIDTH, WINDOW_HEIGHT};
  buffer_t buf;
  draw_context_t ctx;
  buffer_init(&buf, &size, path, 18);
  draw_init_context(&ctx, renderer, &buf.font);
  draw_set_color(&ctx, ctx.background);
  SDL_RenderClear(ctx.renderer);
  buffer_view(&buf, &ctx);
  SDL_RenderPresent(renderer);

  SDL_Event e;
  SDL_StartTextInput();
  int keydowns = 0;

  while (SDL_WaitEvent(&e) == 1) {
    SDL_Rect viewport = {0, 0, buf.size.x, buf.size.y};
    if (e.type == SDL_QUIT) {
      break;
    }

    if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.scancode == SDL_SCANCODE_RETURN && keydowns==0) {
        continue;
      }
      keydowns = 1;
    }

    if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {
        goto render;
      }
      if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        buf.size.x = e.window.data1;
        buf.size.y = e.window.data2;
        goto render;
      }
    }

    bool need_render = buffer_update(&buf, &e);
    if (need_render) goto render;
    continue;

  render: {
      draw_set_color(&ctx, ctx.background);
      SDL_RenderClear(ctx.renderer);
      buffer_view(&buf, &ctx);
      SDL_RenderPresent(renderer);
      continue;
    }
  }

  buffer_destroy(&buf);
  SDL_StopTextInput();
  TTF_Quit();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return EXIT_SUCCESS;
}
