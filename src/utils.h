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
#define match(ty, a, body) ({ ty _(it) { switch(it) body;}; _(a); })
#define return_construct(ty, body) {ty _result = body; return _result;}
#define new(__body) ({typeof(__body) *__temp = malloc(sizeof(__body)); *__temp = __body; __temp;})

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
char *basename(char *str);
int dirname(char *str, char *out);
char *extension(char *str);
char *strchr_last(char *str, int c);
char *pathjoin(char *head, char *tail);
char *strclone(char *str);

inline_always bool
is_inside_xy(rect_t rect, int x, int y) {
  if (x < rect.x || x >= rect.x + rect.w) return false;
  if (y < rect.y || y >= rect.y + rect.h) return false;
  return true;
}

inline_always bool
is_inside_point(rect_t rect, point_t point) {
  if (point.x < rect.x || point.x >= rect.x + rect.w) return false;
  if (point.y < rect.y || point.y >= rect.y + rect.h) return false;
  return true;
}
