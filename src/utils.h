#pragma once
#include <stdbool.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define lambda(c_) ({ c_ _;})
#define absurd(format, args...) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args); raise(SIGINT); })
#define absurd0(format) ({ fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__); raise(SIGINT); })
#define inspect(f, exp) fprintf(stderr, "%s %s:%d | " #exp " = " #f "\n", __FUNCTION__, __FILE__, __LINE__, (exp))
#define debug(format, args...) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__, args)
#define debug0(format) fprintf(stderr, "%s %s:%d | " format "\n", __FUNCTION__, __FILE__, __LINE__)
#define assert0(exp) if (!(exp)) { fprintf(stderr, "%s %s:%d | ASSERTION FAILED: " #exp "\n", __FUNCTION__, __FILE__, __LINE__); raise(SIGINT); }
#define exit_if_not(e) if (!e) { fprintf(stderr, "%s:%d | exit_if_not()\n", __FILE__, __LINE__); return EXIT_FAILURE; }
#define inline_always __inline__ __attribute__((always_inline))
#define swap(a, b) {__auto_type temp = a; a = b; b = temp;}

typedef struct {
  int x;
  int y;
} point_t;

typedef struct {
  int x;
  int y;
  int w;
  int h;
} rect_t;

void noop();
char *strchr_last(char *str, int c);
