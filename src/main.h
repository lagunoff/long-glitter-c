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

char* str_takewhile(char *out, char* src, bool (*p)(char *));

#define absurd(format, args...) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args); exit(1); })
#define absurd0(format) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__); exit(1); })
#define inspect(f, exp) fprintf(stderr, "%s %s:%d | " #exp " = " #f "\n", __FUNCTION__, __FILE__, __LINE__, (exp))
#define debug(format, args...) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args)
#define debug0(format) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__)
#define assert0(exp) if (!(exp)) { fprintf(stderr, "%s %s:%d | ASSERTION FAILED: " #exp "\n", __FUNCTION__, __FILE__, __LINE__); exit(1); }
