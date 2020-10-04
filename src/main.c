#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "cursor.h"

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT (WINDOW_WIDTH)
#define lambda(c_) ({ c_ _;})
#define FONT_HEIGHT 22

void get_text_and_rect(SDL_Renderer *renderer, int x, int y, char *text,
                       TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect) {
  int text_width;
  int text_height;
  SDL_Surface *surface;
  SDL_Color fg = {0, 0, 0, 0.87};
  SDL_Color bg = {255, 255, 255, 0};
  surface = TTF_RenderText_Shaded(font, text, fg, bg);
  *texture = SDL_CreateTextureFromSurface(renderer, surface);
  text_width = surface->w;
  text_height = surface->h;
  SDL_FreeSurface(surface);
  rect->x = x;
  rect->y = y;
  rect->w = text_width;
  rect->h = text_height;
}

char* str_takewhile(char *out, char* src, bool (*p)(char *)) {
  char *src_ptr=src;
  char *out_ptr=out;
  for (; p(src_ptr); src_ptr++, out_ptr++) *out_ptr = *src_ptr;
  *out_ptr='\0';
  return src_ptr;
}

int main(int argc, char **argv) {
  SDL_Renderer *renderer;
  SDL_Window *window;
  int width=WINDOW_WIDTH;
  int height=WINDOW_HEIGHT;

  struct cursor cur = {x0: 0, pos: 0};
  struct scroll scrollY = {pos: 0};

  if (argc < 2) {
    fputs("Need at least one argument\n", stderr);
    return EXIT_FAILURE;
  }

  struct stat st;
  int fd = open(argv[1], O_RDONLY);
  fstat(fd, &st);
  char *maped = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED,fd, 0);
  scroll_down(&scrollY, maped, st.st_size, 10);
  printf("scrollY.pos = %d\n", scrollY.pos);

  SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
  TTF_Init();

  int font_size = 16;
  char* font_path = "/home/vlad/downloads/ttf/Hack-Regular.ttf";
  TTF_Font *font = TTF_OpenFont(font_path, font_size);

  if (font == NULL) {
    fprintf(stderr, "error: font not found\n");
    exit(EXIT_FAILURE);
  }

  void render() {
    SDL_Rect rect;
    SDL_Texture *texture1;
    char buf[1024 * 16]; // Maximum line length — 16kb
    char *iter = maped + scrollY.pos;
    char *end = maped + st.st_size;
    int y=0;
    char *cur_ptr = maped + cur.pos;
    int wX,hX;
    TTF_SizeText(font,"X",&wX,&hX);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    SDL_RenderClear(renderer);

    for(;;) {
      char *iter0 = iter;

      iter = str_takewhile(buf, iter, lambda(bool _(char *c) {
        return (*c != '\n') && (c < end);
      }));
      printf("buf: %s\n", maped);

      if (*buf != '\0' && *buf != '\n') {
        get_text_and_rect(renderer, 0, y, buf, font, &texture1, &rect);
        SDL_RenderCopy(renderer, texture1, NULL, &rect);
        SDL_DestroyTexture(texture1);
      }

      if (cur_ptr >= iter0 && cur_ptr <= iter) {
        int w,w0,h;
        int cur_x_offset = cur_ptr - iter0;
        if (*cur_ptr == '\0' || *cur_ptr == '\n') {
          w0=wX; h=hX;
        } else {
          buf[cur_x_offset + 1]='\0';
          TTF_SizeText(font,buf+cur_x_offset,&w0,&h);
        }
        buf[cur_x_offset]='\0';
        TTF_SizeText(font,buf,&w,&h);
        SDL_Rect cursor_rect = {x:w,y:y,w:w0,h:rect.h};
        printf("cursor_rect: x,y,w,h: %d %d %d %d\n", w,y,w0,rect.h);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderFillRect(renderer, &cursor_rect);
      }

      y += hX;
      if (iter >= end || y > height) break;
      iter++;
    }

    SDL_RenderPresent(renderer);
  }

  render();

  SDL_Event e;

  while (SDL_WaitEvent(&e) == 1) {
    if (e.type == SDL_QUIT) {
      break;
    }

    if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_EXPOSED) {
        render();
        continue;
      }
      if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        width = e.window.data1;
        height = e.window.data2;
        render();
        continue;
      }
    }

    if (e.type == SDL_KEYDOWN) {
      if ( e.key.keysym.scancode == SDL_SCANCODE_LEFT
        || e.key.keysym.scancode == SDL_SCANCODE_B && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        cursor_left(&cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT
        || e.key.keysym.scancode == SDL_SCANCODE_F && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        cursor_right(&cur, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_DOWN && e.key.keysym.mod == 0
        || e.key.keysym.scancode == SDL_SCANCODE_N && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        cursor_down(&cur, maped, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_UP && e.key.keysym.mod == 0
        || e.key.keysym.scancode == SDL_SCANCODE_P && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        cursor_up(&cur, maped);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_F && (e.key.keysym.mod & KMOD_ALT)) {
        cursor_forward_word(&cur, maped, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_B && (e.key.keysym.mod & KMOD_ALT)) {
        cursor_backward_word(&cur, maped);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_E && (e.key.keysym.mod & KMOD_CTRL)) {
        cursor_eol(&cur, maped, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_A && (e.key.keysym.mod & KMOD_CTRL)) {
        cursor_bol(&cur, maped);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_EQUALS  && (e.key.keysym.mod & KMOD_CTRL)) {
        TTF_CloseFont(font);
        font = TTF_OpenFont(font_path, ++font_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_MINUS && (e.key.keysym.mod & KMOD_CTRL)) {
        TTF_CloseFont(font);
        font = TTF_OpenFont(font_path, --font_size);
        render();
        continue;
      }
    }
  }

  /* Deinit TTF. */
  TTF_CloseFont(font);
  TTF_Quit();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}
