#pragma once

#include "parser.h"
#include "draw.h"
#include "input.h"

typedef enum {
  C_MODE_NORMAL,
  C_MODE_SINGLE_COMMENT,
  C_MODE_MULTI_COMMENT,
  C_MODE_KEYWORD,
  C_MODE_CONTROL_FLOW,
  C_MODE_TYPES,
  C_MODE_PREPROCESSOR,
  C_MODE_STRING,
  C_MODE_CONSTANT,
} c_mode_token_t;

typedef struct {
  c_mode_token_t token;
} c_mode_state_t;

highlighter_t c_mode_highlighter;

void c_mode_init(c_mode_state_t *self);
SDL_Color c_mode_choose_color(widget_context_t *self, c_mode_token_t tok);
void c_mode_highlight(c_mode_state_t *self, highlighter_args_t *args, highlighter_cb_t cb);
void c_mode_fast_forward(c_mode_state_t *self, char *input, int len);
