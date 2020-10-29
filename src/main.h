#pragma once

#include <SDL2/SDL.h>
#include <stdbool.h>

#define WINDOW_HEIGHT 711
#define WINDOW_WIDTH (WINDOW_HEIGHT * 4 / 3)
#define FONT_HEIGHT 22

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define lambda(c_) ({ c_ _;})

#define absurd(format, args...) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args); raise(SIGINT); })
#define absurd0(format) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__); raise(SIGINT); })
#define inspect(f, exp) fprintf(stderr, "%s %s:%d | " #exp " = " #f "\n", __FUNCTION__, __FILE__, __LINE__, (exp))
#define debug(format, args...) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args)
#define debug0(format) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__)
#define assert0(exp) if (!(exp)) { fprintf(stderr, "%s %s:%d | ASSERTION FAILED: " #exp "\n", __FUNCTION__, __FILE__, __LINE__); raise(SIGINT); }
