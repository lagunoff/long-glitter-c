#include <stdbool.h>
#include <ctype.h>

#include "c-mode.h"
#include "utils.h"

int c_mode_match_strings(char *input, int input_len, char **arr, int arr_len);
syntax_style_t c_mode_token_to_syntax(c_mode_token_t token);

static char *c_mode_control_flow[] = {
  "break",
  "case",
  "continue",
  "default",
  "do",
  "else",
  "for",
  "goto",
  "if",
  "return",
  "switch",
  "while",
};

static char *c_mode_keywords[] = {
  "enum",
  "extern",
  "inline",
  "sizeof",
  "struct",
  "typedef",
  "union",
  "_Alignas",
  "_Alignof",
  "_Atomic",
  "_Noreturn",
  "_Static_assert",
  "_Thread_local",
};

static char *c_mode_types[] = {
  "auto",
  "char",
  "const",
  "double",
  "float",
  "int",
  "long",
  "register",
  "restrict",
  "short",
  "signed",
  "static",
  "unsigned",
  "void",
  "volatile",
  "int8_t",
  "int16_t",
  "int32_t",
  "int64_t",
  "uint8_t",
  "uint16_t",
  "uint32_t",
  "uint64_t",
  "int_least8_t",
  "int_least16_t",
  "int_least32_t",
  "int_least64_t",
  "uint_least8_t",
  "uint_least16_t",
  "uint_least32_t",
  "uint_least64_t",
  "int_fast8_t",
  "int_fast16_t",
  "int_fast32_t",
  "int_fast64_t",
  "uint_fast8_t",
  "uint_fast16_t",
  "uint_fast32_t",
  "uint_fast64_t",
  "size_t",
  "ssize_t",
  "wchar_t",
  "intptr_t",
  "uintptr_t",
  "intmax_t",
  "uintmax_t",
  "ptrdiff_t",
  "sig_atomic_t",
  "wint_t",
  "_Bool",
  "bool",
  "_Complex",
  "complex",
  "_Imaginary",
  "imaginary",
  "_Generic",
  "va_list",
  "FILE",
  "fpos_t",
  "time_t",
  "max_align_t",
};

void c_mode_reset(void *self) {
  __auto_type ctx = (c_mode_state_t *)self;
  ctx->token = CMode_Normal;
}

void c_mode_highlight(void *self, highlighter_args_t *args, highlighter_t cb) {
  point_t range = {0, 0};
  __auto_type ctx = (c_mode_state_t *)self;
  __auto_type prev_state = ctx->token;
  __auto_type keyword_eaten = 0;

  for (;;) {
    __auto_type iter = args->input + range.y;
    switch (ctx->token) {
    case CMode_Keyword: {
      if (keyword_eaten) {
        range.y += keyword_eaten;
        ctx->token = CMode_Normal;
        goto _continue;
      } else {
        // TODO
      }
      break;
    }
    case CMode_ControlFlow: {
      if (keyword_eaten) {
        range.y += keyword_eaten;
        ctx->token = CMode_Normal;
        goto _continue;
      } else {
        // TODO
      }
      break;
    }
    case CMode_Types: {
      if (keyword_eaten) {
        range.y += keyword_eaten;
        ctx->token = CMode_Normal;
        goto _continue;
      } else {
        // TODO
      }
      break;
    }
    case CMode_Constant: {
      if (!isdigit(*iter)) {
        ctx->token = CMode_Normal;
        goto _continue;
      }
      break;
    }
    case CMode_SingleComment: {
      range.y = args->len;
      ctx->token = CMode_Normal;
      goto _continue;
    }
    case CMode_MultiComment: {
      // SEEK END SEQ
      __auto_type curr_char = *iter;
      __auto_type next_char = iter == args->input + args->len ? ' ' : *(iter + 1);
      if (curr_char == '*' && next_char == '/') {
        range.y += 2;
        ctx->token = CMode_Normal;
        goto _continue;
      }
      break;
    }
    case CMode_String: {
      __auto_type curr_char = *iter;
      __auto_type prev_char = iter == args->input ? ' ' : *(iter - 1);
      if (curr_char == '"' && prev_char != '\\') {
        ctx->token = CMode_Normal;
      }
      break;
    }
    case CMode_Preprocessor: {
      if (!isalnum(*iter)) {
        ctx->token = CMode_Normal;
        goto _continue;
      }
      break;
    }
    case CMode_Normal: {
      // Lookup for start of other syntax modes
      __auto_type last_char = iter == args->input ? ' ' : *(iter - 1);
      __auto_type curr_char = *iter;
      __auto_type next_char = iter == args->input + args->len ? ' ' : *(iter + 1);
      if (isalnum(last_char)) break;
      if (isdigit(curr_char)) {
        ctx->token = CMode_Constant;
        break;
      }
      if (curr_char == '/' && next_char == '/') {
        ctx->token = CMode_SingleComment;
        goto _continue;
      }
      if (curr_char == '/' && next_char == '*') {
        ctx->token = CMode_MultiComment;
        goto _continue;
      }
      if (curr_char == '#') {
        ctx->token = CMode_Preprocessor;
        goto _continue;
      }
      if (curr_char == '"') {
        ctx->token = CMode_String;
        goto _continue;
      }
      keyword_eaten = c_mode_match_strings(iter, args->len - (iter - args->input), c_mode_keywords, sizeof(c_mode_keywords)/sizeof(c_mode_keywords[0]));
      if (keyword_eaten) { ctx->token = CMode_Keyword; goto _continue; }
      keyword_eaten = c_mode_match_strings(iter, args->len - (iter - args->input), c_mode_control_flow, sizeof(c_mode_control_flow)/sizeof(c_mode_control_flow[0]));
      if (keyword_eaten) { ctx->token = CMode_ControlFlow; goto _continue; }
      keyword_eaten = c_mode_match_strings(iter, args->len - (iter - args->input), c_mode_types, sizeof(c_mode_types)/sizeof(c_mode_types[0]));
      if (keyword_eaten) { ctx->token = CMode_Types; goto _continue; }
      break;
    }}
    _continue:
    if (prev_state != ctx->token || iter == args->input + args->len) {
      if (range.y - range.x) {
        args->normal->syntax = c_mode_token_to_syntax(prev_state);
        cb(range, args->normal);
      }
      range.x = range.y;
      prev_state = ctx->token;
    }
    if (!(iter < args->input + args->len)) break;
    range.y++;
  }
}

void c_mode_fast_forward(void *self, char *input, int len) {
  /* c_mode_highlight(ctx, input, len, lambda(void _(char *start, int len, c_mode_token_t state){ */
  /* })); */
}

int c_mode_match_strings(char *input, int input_len, char **arr, int arr_len) {
  for (int i = 0; i < arr_len; i++) {
    __auto_type needle = arr[i];
    __auto_type needle_len = strlen(needle);
    __auto_type matches = true;
    __auto_type eof = needle_len >= input_len;
    if (eof) continue;
    int j = 0;
    for (; j < needle_len; j++) {
      if (input[j] != needle[j]) {
        matches = false;
        break;
      }
    }
    if (matches) return j;
  }
  return 0;
}

syntax_style_t c_mode_token_to_syntax(c_mode_token_t token) {
  switch (token) {
  case CMode_Normal: return Syntax_Normal;
  case CMode_SingleComment: return Syntax_Comment;
  case CMode_MultiComment: return Syntax_Comment;
  case CMode_Keyword: return Syntax_Keyword;
  case CMode_ControlFlow: return Syntax_Builtin;
  case CMode_Types: return Syntax_Type;
  case CMode_Preprocessor: return Syntax_Preprocessor;
  case CMode_String: return Syntax_String;
  case CMode_Constant: return Syntax_Constant;
  }
}

static __attribute__((constructor)) void __init__() {
  c_mode_highlighter.reset = &c_mode_reset;
  c_mode_highlighter.forward = &c_mode_fast_forward;
  c_mode_highlighter.highlight = &c_mode_highlight;
}
