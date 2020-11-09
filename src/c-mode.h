#pragma once

#include "parser.h"
#include "draw.h"
#include "buff-string.h"

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
} c_mode_state_t;

typedef struct {
  c_mode_state_t     state;
} c_mode_context_t;

typedef void (*c_mode_result_t)(char *input, int len, c_mode_state_t state);
typedef void (*c_mode_parser_t)(c_mode_context_t *ctx, int len, c_mode_result_t result);

void c_mode_init(c_mode_context_t *ctx);
SDL_Color c_mode_choose_color(draw_context_t *ctx, c_mode_state_t state);
void c_mode_highlight(c_mode_context_t *ctx, char *input, int len, c_mode_result_t result);
void c_mode_fast_forward(c_mode_context_t *ctx, char *input, int len);
