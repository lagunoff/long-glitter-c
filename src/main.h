#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT (WINDOW_WIDTH)
#define FONT_HEIGHT 22
#define lambda(c_) ({ c_ _;})

struct loaded_font {
  TTF_Font *font;
  int font_size;
  int X_width;
  int X_height;
};

void get_text_and_rect(
  SDL_Renderer *renderer, int x, int y, char *text,
  TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect
);

void init_font(struct loaded_font *lf, int font_size);

char* str_takewhile(char *out, char* src, bool (*p)(char *));
