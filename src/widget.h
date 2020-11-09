#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

#include "draw.h"

typedef enum {
  MSG_NOOP,
  MSG_MEASURE,
  MSG_VIEW,
  MSG_FREE,
  MSG_SDL_EVENT,
  MSG_USER,
} widget_msg_tag_t;

typedef union {
  SDL_Point      *measure;
  SDL_Event      *sdl_event;
  draw_context_t *view;
} widget_msg_data_t;

typedef struct {
  widget_msg_tag_t tag;
} widget_noop_t;

typedef struct {
  widget_msg_tag_t tag;
} widget_view_t;

typedef struct {
  widget_msg_tag_t tag;
} widget_free_t;

typedef struct {
  widget_msg_tag_t tag;
  SDL_Point       *result;
} widget_measure_t;

typedef struct {
  widget_msg_tag_t tag;
  SDL_Event       *event;
} widget_sdl_event_t;

typedef union {
  widget_msg_tag_t   tag;
  widget_noop_t      noop;
  widget_view_t      view;
  widget_free_t      free;
  widget_measure_t   measure;
  widget_sdl_event_t sdl_event;
} widget_msg_t;

typedef void (*yield_t)(void *msg);
typedef void (*widget_t)(void *self, widget_msg_t *msg, yield_t yield);

widget_msg_t msg_noop;
widget_msg_t msg_free;
widget_msg_t msg_view;
widget_msg_t msg_measure(SDL_Point *result);
widget_msg_t msg_sdl_event(SDL_Event *event);

void widget_window_set(SDL_Window *window, const widget_t widget, const void *model);
void widget_window_get(SDL_Window *window, widget_t *widget, void **model);
SDL_Window *widget_get_event_window(SDL_Event *e);
void widget_open_window(SDL_Rect *size_pos, Uint32 window_flags, SDL_Window **window, SDL_Renderer **renderer, widget_t widget, void *model);
void widget_open_window_measure(SDL_Point *pos, Uint32 window_flags, SDL_Window **window, SDL_Renderer **renderer, widget_t widget, void *model);
void widget_close_window(SDL_Window *window);
