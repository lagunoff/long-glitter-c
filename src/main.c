#include <stdlib.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "buff-string.h"
#include "cursor.h"
#include "main.h"

#define MODIFY_CURSOR(f, c, args...) ({ struct cursor __old = *(c); f((c), args); cursor_modified(&scrollY, &__old, (c), &lf, height, buf, st.st_size); })

int main(int argc, char **argv) {
  SDL_Renderer *renderer;
  SDL_Window *window;
  int width=WINDOW_WIDTH;
  int height=WINDOW_HEIGHT;

  struct cursor cur = {x0: 0, pos: 0};
  struct scroll scrollY = {pos: 0};
  struct loaded_font lf = {0};

  if (argc < 2) {
    fputs("Need at least one argument\n", stderr);
    return EXIT_FAILURE;
  }

  struct stat st;
  int fd = open(argv[1], O_RDONLY);
  fstat(fd, &st);
  char *buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED,fd, 0);

  struct buff_string str = {buf, st.st_size, NULL};
  struct buff_string_iter iter = BUFF_STRING_BEGIN(&str);

  SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE|SDL_WINDOW_MAXIMIZED, &window, &renderer);
  TTF_Init();

  init_font(&lf, 16);

  void render() {
    SDL_Rect rect;
    SDL_Texture *texture1;
    char temp[1024 * 16]; // Maximum line length — 16kb
    char *iter = buf + scrollY.pos;
    char *end = buf + st.st_size;
    int y=0;
    char *cur_ptr = buf + cur.pos;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    SDL_RenderClear(renderer);

    for(;;) {
      char *iter0 = iter;

      iter = str_takewhile(temp, iter, lambda(bool _(char *c) {
        return (*c != '\n') && (c < end);
      }));

      if (*temp != '\0' && *temp != '\n') {
        get_text_and_rect(renderer, 0, y, temp, lf.font, &texture1, &rect);
        SDL_RenderCopy(renderer, texture1, NULL, &rect);
        SDL_DestroyTexture(texture1);
      }

      if (cur_ptr >= iter0 && cur_ptr <= iter) {
        int w,h;
        int cur_x_offset = cur_ptr - iter0;
        temp[cur_x_offset]='\0';
        TTF_SizeText(lf.font,temp,&w,&h);
        SDL_Rect cursor_rect = {x:w,y:y,w:lf.X_width,h:lf.X_height};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderFillRect(renderer, &cursor_rect);
      }

      y += lf.X_height;
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
        MODIFY_CURSOR(cursor_left, &cur, buf);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT
        || e.key.keysym.scancode == SDL_SCANCODE_F && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        MODIFY_CURSOR(cursor_right, &cur, buf, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_DOWN && e.key.keysym.mod == 0
        || e.key.keysym.scancode == SDL_SCANCODE_N && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        MODIFY_CURSOR(cursor_down, &cur, buf, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_UP && e.key.keysym.mod == 0
        || e.key.keysym.scancode == SDL_SCANCODE_P && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        MODIFY_CURSOR(cursor_up, &cur, buf);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_F && (e.key.keysym.mod & KMOD_ALT)) {
        MODIFY_CURSOR(cursor_forward_word, &cur, buf, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_B && (e.key.keysym.mod & KMOD_ALT)) {
        MODIFY_CURSOR(cursor_backward_word, &cur, buf);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_E && (e.key.keysym.mod & KMOD_CTRL)) {
        MODIFY_CURSOR(cursor_eol, &cur, buf, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_A && (e.key.keysym.mod & KMOD_CTRL)) {
        MODIFY_CURSOR(cursor_bol, &cur, buf);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_V && (e.key.keysym.mod & KMOD_CTRL)
        || e.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN && (e.key.keysym.mod==0)
      ) {
        scroll_page(&scrollY, &cur, &lf, height, buf, st.st_size, 1);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_V && (e.key.keysym.mod & KMOD_ALT)
        || e.key.keysym.scancode == SDL_SCANCODE_PAGEUP && (e.key.keysym.mod==0)
      ) {
        scroll_page(&scrollY, &cur, &lf, height, buf, st.st_size, -1);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET && (e.key.keysym.mod & (KMOD_ALT | KMOD_SHIFT))) {
        MODIFY_CURSOR(backward_paragraph, &cur, buf);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET && (e.key.keysym.mod & (KMOD_ALT | KMOD_SHIFT))) {
        MODIFY_CURSOR(forward_paragraph, &cur, buf, st.st_size);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_EQUALS  && (e.key.keysym.mod & KMOD_CTRL)) {
        TTF_CloseFont(lf.font);
        init_font(&lf, lf.font_size + 1);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_MINUS && (e.key.keysym.mod & KMOD_CTRL)) {
        TTF_CloseFont(lf.font);
        init_font(&lf, lf.font_size - 1);
        render();
        continue;
      }
    }
  }

  TTF_CloseFont(lf.font);
  TTF_Quit();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}

void init_font(struct loaded_font *lf, int font_size) {
  lf->font_size = font_size;
  lf->font = TTF_OpenFont("/home/vlad/downloads/ttf/Hack-Regular.ttf", font_size);
  TTF_SizeText(lf->font,"X",&lf->X_width,&lf->X_height);
  if (lf->font == NULL) {
    fprintf(stderr, "error: font not found\n");
    exit(EXIT_FAILURE);
  }
}

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

char* str_takewhile(char *dest, char* src, bool (*p)(char *)) {
  char *src_ptr=src;
  char *dest_ptr=dest;
  for (; p(src_ptr); src_ptr++, dest_ptr++) *dest_ptr = *src_ptr;
  *dest_ptr='\0';
  return src_ptr;
}
