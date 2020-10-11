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

#define MODIFY_CURSOR(f, cu) ({ struct cursor __old = *(cu); f(cu); cursor_modified(&scrollY, &__old, (cu), &lf, height); })

int main(int argc, char **argv) {
  SDL_Renderer *renderer;
  SDL_Window *window;
  int width=WINDOW_WIDTH;
  int height=WINDOW_HEIGHT;

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

  struct stat st;
  int fd = open(argv[1], O_RDONLY);
  fstat(fd, &st);
  char *contents = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED,fd, 0);

  struct buff_string buf = {contents, st.st_size, NULL, NULL};
  struct cursor cur = {x0: 0, pos: {0}};
  struct scroll scrollY = {pos: {0}};
  struct loaded_font lf = {0};
  buff_string_begin(&(cur.pos), &buf);
  buff_string_begin(&(scrollY.pos), &buf);

  init_font(&lf, 16);

  void render() {
    SDL_Rect rect;
    SDL_Texture *texture1;
    char temp[1024 * 16]; // Maximum line length — 16kb
    struct buff_string_iter iter = scrollY.pos;
    int y=0;
    int cursor_offset = buff_string_offset(&(cur.pos));

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
    SDL_RenderClear(renderer);

    char temp1[1024 * 16];
    char temp2[1024 * 16];
    struct buff_string_iter iter1 = scrollY.pos;

    buff_string_takewhile(&iter1, temp1, lambda(bool _(char c) {
          return c != '\n';
    }));
    iter1 = cur.pos;
    buff_string_takewhile(&iter1, temp2, lambda(bool _(char c) {
          return c != '\n';
    }));

    for(;;) {
      int offset0 = buff_string_offset(&iter);

      buff_string_takewhile(&iter, temp, lambda(bool _(char c) {
        return c != '\n';
      }));

      if (*temp != '\0' && *temp != '\n') {
        get_text_and_rect(renderer, 0, y, temp, lf.font, &texture1, &rect);
        SDL_RenderCopy(renderer, texture1, NULL, &rect);
        SDL_DestroyTexture(texture1);
      }

      int iter_offset = buff_string_offset(&iter);

      if (cursor_offset >= offset0 && cursor_offset <= iter_offset) {
        int w,h;
        int cur_x_offset = cursor_offset - offset0;
        temp[cur_x_offset]='\0';
        TTF_SizeText(lf.font,temp,&w,&h);
        if (cur_x_offset == 0) w=0;
        SDL_Rect cursor_rect = {x:w,y:y,w:lf.X_width,h:lf.X_height};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderFillRect(renderer, &cursor_rect);
      }

      y += lf.X_height;
      if (y > height) break;
      buff_string_move(&iter, 1);
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
        MODIFY_CURSOR(cursor_left, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_RIGHT
        || e.key.keysym.scancode == SDL_SCANCODE_F && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        MODIFY_CURSOR(cursor_right, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_DOWN && e.key.keysym.mod == 0
        || e.key.keysym.scancode == SDL_SCANCODE_N && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        MODIFY_CURSOR(cursor_down, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_UP && e.key.keysym.mod == 0
        || e.key.keysym.scancode == SDL_SCANCODE_P && (e.key.keysym.mod & KMOD_CTRL)
      ) {
        MODIFY_CURSOR(cursor_up, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_F && (e.key.keysym.mod & KMOD_ALT)) {
        MODIFY_CURSOR(cursor_forward_word, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_B && (e.key.keysym.mod & KMOD_ALT)) {
        MODIFY_CURSOR(cursor_backward_word, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_E && (e.key.keysym.mod & KMOD_CTRL)) {
        MODIFY_CURSOR(cursor_eol, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_A && (e.key.keysym.mod & KMOD_CTRL)) {
        MODIFY_CURSOR(cursor_bol, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_V && (e.key.keysym.mod & KMOD_CTRL)
        || e.key.keysym.scancode == SDL_SCANCODE_PAGEDOWN && (e.key.keysym.mod==0)
      ) {
        scroll_page(&scrollY, &cur, &lf, height, 1);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_V && (e.key.keysym.mod & KMOD_ALT)
        || e.key.keysym.scancode == SDL_SCANCODE_PAGEUP && (e.key.keysym.mod==0)
      ) {
        scroll_page(&scrollY, &cur, &lf, height, -1);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_LEFTBRACKET && (e.key.keysym.mod & (KMOD_ALT | KMOD_SHIFT))) {
        MODIFY_CURSOR(backward_paragraph, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_RIGHTBRACKET && (e.key.keysym.mod & (KMOD_ALT | KMOD_SHIFT))) {
        MODIFY_CURSOR(forward_paragraph, &cur);
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
      if (e.key.keysym.scancode == SDL_SCANCODE_PERIOD && (e.key.keysym.mod  & (KMOD_ALT | KMOD_SHIFT))) {
        MODIFY_CURSOR(cursor_end, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_COMMA && (e.key.keysym.mod  & (KMOD_ALT | KMOD_SHIFT))) {
        MODIFY_CURSOR(cursor_begin, &cur);
        render();
        continue;
      }
      if (e.key.keysym.scancode == SDL_SCANCODE_A && (e.key.keysym.mod == 0)) {
        buff_string_insert(&(cur.pos), "A", 0, &(scrollY.pos), NULL);
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
