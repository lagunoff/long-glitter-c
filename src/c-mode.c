#include <stdbool.h>

#include "c-mode.h"
#include "main.h"

int c_mode_match_strings(char *input, int input_len, char **arr, int arr_len);

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

void c_mode_init(c_mode_context_t *ctx) {
  ctx->state = CMODE_NORMAL;
}

SDL_Color c_mode_choose_color(draw_context_t *ctx, c_mode_state_t state) {
  switch (state) {
  case CMODE_NORMAL: return ctx->palette->primary_text;
  case CMODE_SINGLE_COMMENT: return ctx->palette->syntax.comment;
  case CMODE_MULTI_COMMENT: return ctx->palette->syntax.comment;
  case CMODE_KEYWORD: return ctx->palette->syntax.keyword;
  case CMODE_CONTROL_FLOW: return ctx->palette->syntax.builtin;
  case CMODE_TYPES: return ctx->palette->syntax.identifier;
  case CMODE_PREPROCESSOR: return ctx->palette->syntax.preprocessor;
  case CMODE_STRING: return ctx->palette->syntax.string;
  case CMODE_CONSTANT: return ctx->palette->syntax.constant;
  }
}

void c_mode_highlight(c_mode_context_t *ctx, char *input, int len, c_mode_result_t result) {
  char *prefix_start = input;
  char *iter = input;
  c_mode_state_t state = CMODE_NORMAL;
  for (;iter < input + len;) {
    int eaten = 0;
    switch (ctx->state) {
    case CMODE_KEYWORD: {
      // Seek end of word
      break;
    }
    case CMODE_CONTROL_FLOW: {
      // Seek end of word
      break;
    }
    case CMODE_TYPES: {
      // Seek end of word
      break;
    }
    case CMODE_CONSTANT: {
      __auto_type curr_char = *iter;
      if (!isdigit(*iter)) {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, CMODE_CONSTANT);
        ctx->state = CMODE_NORMAL;
        prefix_start = iter;
        continue;
      }
      break;
    }
    case CMODE_SINGLE_COMMENT: {
      // SEEK NEWLINE
      __auto_type comment_len = len - (iter - input);
      if (comment_len) result(iter, comment_len, CMODE_SINGLE_COMMENT);
      iter += comment_len;
      prefix_start = iter;
      ctx->state = CMODE_NORMAL;
      break;
    }
    case CMODE_MULTI_COMMENT: {
      // SEEK END SEQ
      __auto_type curr_char = *iter;
      __auto_type next_char = iter == input + len ? ' ' : *(iter + 1);
      if (curr_char == '*' && next_char == '/') {
        iter += 2;
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, CMODE_MULTI_COMMENT);
        ctx->state = CMODE_NORMAL;
        prefix_start = iter;
        continue;
      }
      break;
    }
    case CMODE_STRING: {
      __auto_type curr_char = *iter;
      __auto_type prev_char = iter == input ? ' ' : *(iter - 1);
      if (curr_char == '"' && prev_char != '\\') {
        iter += 1;
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, CMODE_STRING);
        ctx->state = CMODE_NORMAL;
        prefix_start = iter;
        continue;
      }
      break;
    }
    case CMODE_PREPROCESSOR: {
      if (!isalnum(*iter)) {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, CMODE_PREPROCESSOR);
        ctx->state = CMODE_NORMAL;
        prefix_start = iter;
        continue;
      }
      break;
    }
    case CMODE_NORMAL: {
      // Lookup for start of other syntax modes
      __auto_type last_char = iter == input ? ' ' : *(iter - 1);
      __auto_type curr_char = *iter;
      __auto_type next_char = iter == input + len ? ' ' : *(iter + 1);
      if (isalnum(last_char)) break;
      if (isdigit(curr_char)) {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, ctx->state);
        prefix_start = iter;
        ctx->state = CMODE_CONSTANT;
        continue;
      }
      if (curr_char == '/' && next_char == '/') {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, ctx->state);
        ctx->state = CMODE_SINGLE_COMMENT;
        continue;
      }
      if (curr_char == '/' && next_char == '*') {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, ctx->state);
        prefix_start = iter;
        iter += 2;
        ctx->state = CMODE_MULTI_COMMENT;
        continue;
      }
      if (curr_char == '#') {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, ctx->state);
        prefix_start = iter;
        iter += 1;
        ctx->state = CMODE_PREPROCESSOR;
        continue;
      }
      if (curr_char == '"') {
        __auto_type prefix_len = iter - prefix_start;
        if (prefix_len) result(prefix_start, prefix_len, ctx->state);
        prefix_start = iter;
        iter += 1;
        ctx->state = CMODE_STRING;
        continue;
      }
      eaten = c_mode_match_strings(iter, len - (iter - input), c_mode_keywords, sizeof(c_mode_keywords)/sizeof(c_mode_keywords[0]));
      if (eaten) { state = CMODE_KEYWORD; goto eaten_some; }
      eaten = c_mode_match_strings(iter, len - (iter - input), c_mode_control_flow, sizeof(c_mode_control_flow)/sizeof(c_mode_control_flow[0]));
      if (eaten) { state = CMODE_CONTROL_FLOW; goto eaten_some; }
      eaten = c_mode_match_strings(iter, len - (iter - input), c_mode_types, sizeof(c_mode_types)/sizeof(c_mode_types[0]));
      if (eaten) { state = CMODE_TYPES; goto eaten_some; }
      break;
    }
    }
    iter++;
    continue;
  eaten_some: {
      __auto_type prefix_len = iter - prefix_start;
      if (prefix_len) {
        result(prefix_start, prefix_len, ctx->state);
      }
      result(iter, eaten, state);
      ctx->state = CMODE_NORMAL;
      iter = iter + eaten;
      prefix_start = iter;
    }
  }
  __auto_type prefix_len = iter - prefix_start;
  if (prefix_len) {
    result(prefix_start, prefix_len, ctx->state);
  }
}


void c_mode_fast_forward(c_mode_context_t *ctx, char *input, int len) {
  c_mode_highlight(ctx, input, len, lambda(void _(char *start, int len, c_mode_state_t state){
  }));
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

int c_mode_unittest() {
}
