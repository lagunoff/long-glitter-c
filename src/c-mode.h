#pragma once

#include "parser.h"
#include "graphics.h"
#include "input.h"

typedef enum {
  CMode_Normal,
  CMode_SingleComment,
  CMode_MultiComment,
  CMode_Keyword,
  CMode_ControlFlow,
  CMode_Types,
  CMode_Preprocessor,
  CMode_String,
  CMode_Constant,
} c_mode_token_t;

typedef struct {
  c_mode_token_t token;
} c_mode_state_t;

syntax_highlighter_t c_mode_highlighter;

void c_mode_reset(void *self);
void c_mode_highlight(void *self, highlighter_args_t *args, highlighter_t cb);
void c_mode_fast_forward(void *self, char *input, int len);
