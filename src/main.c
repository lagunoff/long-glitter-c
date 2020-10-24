#include <stdlib.h>

#include <SDL2/SDL.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cairo.h>

#include "buff-string.h"
#include "cursor.h"
#include "buffer.h"
#include "main.h"


int main(int argc, char **argv) {
  SDL_Renderer *renderer;
  SDL_Window *window;
  int fridge = 8;
  char *path = argc < 2 ? "/home/vlad/job/long-glitter-c/test/01.lisp" : argv[1];

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  if (SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED, &window, &renderer) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Point size = {WINDOW_WIDTH - fridge, WINDOW_HEIGHT};
  buffer_t buf;
  buffer_init(&buf, &size, path);
  SDL_Texture *texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    buf.size.x, buf.size.y
  );

  void *pixels;
  int pitch;
  SDL_LockTexture(texture, NULL, &pixels, &pitch);

  cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
    pixels,
    CAIRO_FORMAT_ARGB32,
    buf.size.x, buf.size.y, pitch
  );
  cairo_t *cr = cairo_create(cairo_surface);

  buffer_view(&buf, cr);
  SDL_UnlockTexture(texture);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
  cairo_surface_finish(cairo_surface);
  cairo_destroy (cr);

  SDL_Event e;
  SDL_StartTextInput();
  int keydowns = 0;

  while (SDL_WaitEvent(&e) == 1) {
    SDL_Rect viewport = {fridge, 0, buf.size.x, buf.size.y};
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
        inspect(%d, e.window.data1);
        inspect(%d, e.window.data2);
        buf.size.x = e.window.data1 - fridge;
        buf.size.y = e.window.data2;
        SDL_DestroyTexture(texture);
        texture = SDL_CreateTexture(
          renderer,
          SDL_PIXELFORMAT_ARGB8888,
          SDL_TEXTUREACCESS_STREAMING,
          buf.size.x, buf.size.y
        );
        goto render;
      }
    }

    bool need_render = buffer_update(&buf, &e);
    if (need_render) goto render;
    continue;

  render: {
      void *pixels;
      int pitch;
      SDL_LockTexture(texture, NULL, &pixels, &pitch);
      cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
        pixels,
        CAIRO_FORMAT_ARGB32,
        buf.size.x, buf.size.y, pitch
      );
      cairo_t *cr = cairo_create(cairo_surface);
      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_matrix_t matrix;
      cairo_get_matrix(cr, &matrix);
      cairo_translate(cr, fridge, 0);
      cairo_paint (cr);
      buffer_view(&buf, cr);
      cairo_set_matrix(cr, &matrix);
      SDL_UnlockTexture(texture);
      SDL_RenderCopy(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);
      cairo_surface_finish(cairo_surface);
      cairo_destroy(cr);
      continue;
    }
  }

  buffer_destroy(&buf);
  SDL_StopTextInput();

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return EXIT_SUCCESS;
}
