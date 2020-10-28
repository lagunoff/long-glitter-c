#include "widget.h"

void widget_window_set(SDL_Window *window, const widget_t *widget, const void *model) {
  SDL_SetWindowData(window, "widget", (void *)widget);
  SDL_SetWindowData(window, "model", (void *)model);
}

void widget_window_get(SDL_Window *window, widget_t **widget, void **model) {
  *widget = (widget_t *)SDL_GetWindowData(window, "widget");
  *model = SDL_GetWindowData(window, "model");
}
