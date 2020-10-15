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

  if (argc < 2) {
    fputs("Need at least one argument\n", stderr);
    return EXIT_FAILURE;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED, &window, &renderer) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if (TTF_Init() != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Point size = {WINDOW_WIDTH, WINDOW_HEIGHT};
  struct buffer buf;
  buffer_init(&buf, &size, argv[1]);
  buffer_view(&buf, renderer);

  SDL_Event e;
  SDL_StartTextInput();

  while (SDL_WaitEvent(&e) == 1) {
    if (e.type == SDL_QUIT) {
      break;
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

    render:
    buffer_view(&buf, renderer);
    continue;
  }

  buffer_destroy(&buf);
  SDL_StopTextInput();

  TTF_Quit();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
