#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define WINDOW_WIDTH 300
#define WINDOW_HEIGHT (WINDOW_WIDTH)
#define FONT_HEIGHT 22

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

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

#define absurd(format, args...) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args); exit(1); })
#define absurd0(format) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__); exit(1); })
#define inspect(f, exp) fprintf(stderr, "%s %s:%d | " #exp " = " #f "\n", __FUNCTION__, __FILE__, __LINE__, (exp))
#define debug(format, args...) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args)
#define debug0(format) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__)
#define assert0(exp) if (!(exp)) { fprintf(stderr, "%s %s:%d | ASSERTION FAILED: " #exp "\n", __FUNCTION__, __FILE__, __LINE__); exit(1); }
