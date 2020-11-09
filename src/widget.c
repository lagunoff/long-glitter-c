#include "widget.h"
#include "main.h"

void widget_window_set(SDL_Window *window, const widget_t widget, const void *model) {
  SDL_SetWindowData(window, "widget", (void *)widget);
  SDL_SetWindowData(window, "model", (void *)model);
}

void widget_window_get(SDL_Window *window, widget_t *widget, void **model) {
  *widget = (widget_t)SDL_GetWindowData(window, "widget");
  *model = SDL_GetWindowData(window, "model");
}

widget_msg_t msg_measure(SDL_Point *result) {
  widget_msg_t msg = {.measure = {.tag = MSG_MEASURE, .result = result}};
  return msg;
}

widget_msg_t msg_sdl_event(SDL_Event *event) {
  widget_msg_t msg = {.sdl_event = {.tag = MSG_SDL_EVENT, .event = event}};
  return msg;
}

void widget_open_window(
  SDL_Rect      *size_pos,
  Uint32         window_flags,
  SDL_Window   **window,
  SDL_Renderer **renderer,
  widget_t      widget,
  void          *model
) {
  if (SDL_CreateWindowAndRenderer(size_pos->w, size_pos->h, SDL_WINDOW_TOOLTIP, window, renderer) != 0) {
    fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  SDL_SetWindowPosition(*window, size_pos->x, size_pos->y);
  widget_window_set(*window, widget, model);
  SDL_ShowWindow(*window);
}

void widget_close_window(SDL_Window *window) {
  widget_t root_widget = NULL;
  void *root_widget_data = NULL;
  SDL_Renderer *renderer = SDL_GetRenderer(window);
  widget_window_get(window, &root_widget, &root_widget_data);
  if (root_widget && root_widget_data) {
    root_widget(root_widget_data, &msg_free, lambda(void _(void *msg){ return; }));
  }

  SDL_SetWindowData(window, "widget", NULL);
  SDL_SetWindowData(window, "model", NULL);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void widget_open_window_measure(
  SDL_Point       *pos,
  Uint32           window_flags,
  SDL_Window     **window,
  SDL_Renderer   **renderer,
  widget_t widget,
  void            *model
) {
  SDL_Point measured;
  widget_msg_t m[4];
  m[0] = msg_measure(&measured);
  widget(model, m, lambda(void _(void *msg){ return; }));
  SDL_Rect rect = {pos->x, pos->y, measured.x, measured.y};
  widget_open_window(&rect,window_flags, window, renderer, widget, model);
}

SDL_Window *widget_get_event_window(SDL_Event *e) {
  if (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP) {
    return SDL_GetWindowFromID(e->button.windowID);
  }

  if (e->type == SDL_MOUSEMOTION) {
    return SDL_GetWindowFromID(e->motion.windowID);
  }

  if (e->type == SDL_WINDOWEVENT) {
    return SDL_GetWindowFromID(e->window.windowID);
  }

  if (e->type == SDL_TEXTINPUT) {
    return SDL_GetWindowFromID(e->text.windowID);
  }

  if (e->type == SDL_MOUSEWHEEL) {
    return SDL_GetWindowFromID(e->wheel.windowID);
  }

  if (e->type == SDL_KEYDOWN) {
    return SDL_GetWindowFromID(e->key.windowID);
  }

  return NULL;
}

static __attribute__((constructor)) void __init__() {
  msg_view.tag = MSG_VIEW;
  msg_noop.tag = MSG_NOOP;
  msg_free.tag = MSG_FREE;
}
