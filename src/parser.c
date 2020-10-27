#include <stdlib.h>
#include <ctype.h>

#include "parser.h"
#include "main.h"

void p_number(parser_context_t *ctx, parser_result_t *result) {
  expr_t *expr;
  bool minus = false;
  int value = 0;
  int initial_offset = ctx->offset;
  if (ctx->input[ctx->offset]=='-') {
    minus = true;
    ctx->offset++;
  }
  if (ctx->input[ctx->offset]=='_') {
    result->failure.message = "unexpected underscore at the beginning";
    goto failure;
  }
  for(;;ctx->offset++) {
    char ch = ctx->input[ctx->offset];
    if (ch == '_') {
      ctx->offset++; continue;
    }
    if (ctx->offset >= ctx->len || !isdigit(ch)) {
      if (ctx->offset - initial_offset != 0) goto success;
      result->failure.message = ctx->offset >= ctx->len ? "unexpected EOF" : "isdigit(ch) failed";
      goto failure;
    }
    value = value * 10 + ch - '0';
  }
  success:
  expr = (expr_t*)malloc(sizeof(expr_t));
  expr->tag = EXP_LITERAL;
  expr->literal.tag = LITERAL_NUMBER;
  expr->literal.number.value = minus ? -value : value;
  result->tag = PARSER_SUCCESS;
  result->success.value = expr;
  return;
  failure:
  result->tag = PARSER_FAILURE;
}

// FIXME: implement escaping
void p_string(parser_context_t *ctx, parser_result_t *result) {
  expr_t *expr;
  int value = 0;
  int initial_offset = ctx->offset;

  if (ctx->input[ctx->offset]!='\'') {
    result->failure.message = "expected single colon";
    goto failure;
  }
  ctx->offset++;
  for(;;ctx->offset++) {
    char ch = ctx->input[ctx->offset];
    if (ctx->offset >= ctx->len) {
      result->failure.message = "unexpected EOF";
      goto failure;
    }
    if (ch == '\'') {
      goto success;
    }
    value = value * 10 + ch - '0';
  }
  success:
  expr = (expr_t*)malloc(sizeof(expr_t));
  expr->tag = EXP_LITERAL;
  expr->literal.tag = LITERAL_STRING;
  expr->literal.string.len = ctx->offset - initial_offset - 1;
  expr->literal.string.utf8 = (char*)malloc(expr->literal.string.len + 1);
  strncpy(expr->literal.string.utf8, ctx->input + initial_offset + 1, expr->literal.string.len);
  result->tag = PARSER_SUCCESS;
  result->success.value = expr;
  ctx->offset++;
  return;
  failure:
  result->tag = PARSER_FAILURE;
}

// FIXME: implement escaping
void p_literal(parser_context_t *ctx, parser_result_t *result) {
  p_number(ctx, result);
  if (result->tag == PARSER_FAILURE) {
    p_string(ctx, result);
  }
}

void p_space(parser_context_t *ctx, parser_result_t *result) {
  for(;;ctx->offset++) {
    char ch = ctx->input[ctx->offset];
    if (ctx->offset >= ctx->len || !isspace(ch)) {
      goto success;
    }
  }
  success:
  result->tag = PARSER_SUCCESS;
  result->success.value = NULL;
}

void p_indent(parser_context_t *ctx, parser_result_t *result) {
  expr_t *expr;
  int initial_offset = ctx->offset;

  bool isident(char ch) {
    return isalnum(ch) || ch == '_';
  }

  for(;;ctx->offset++) {
    char ch = ctx->input[ctx->offset];
    if (ctx->offset >= ctx->len && initial_offset - ctx->offset == 0) {
      result->failure.message = "unexpected EOF";
      goto failure;
    }
    if (!isident(ch)) {
      if (initial_offset - ctx->offset == 0) {
        result->failure.message = "isident(ch) failed";
        goto failure;
      }
      goto success;
    }
  }
  success:
  expr = (expr_t*)malloc(sizeof(expr_t));
  expr->tag = EXP_IDENT;
  expr->ident.len = ctx->offset - initial_offset;
  expr->ident.utf8 = (char*)malloc(expr->ident.len + 1);
  strncpy(expr->ident.utf8, ctx->input + initial_offset, expr->ident.len);
  result->tag = PARSER_SUCCESS;
  result->success.value = expr;
  ctx->offset++;
  return;
  failure:
  result->tag = PARSER_FAILURE;
}

void p_infix_operator(parser_context_t *ctx, parser_result_t *result) {
  if (ctx->offset >= ctx->len) {
    result->failure.message = "unexpected EOF";
    goto failure;
  }
  char ch = ctx->input[ctx->offset];
  if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
    result->success.value = (void *)(intptr_t)ch;
    goto success;
  }
  success:
  result->tag = PARSER_SUCCESS;
  ctx->offset++;
  return;
  failure:
  result->tag = PARSER_FAILURE;
}

void p_expr(parser_context_t *ctx, parser_result_t *result) {
  parser_result_t ignore;
  parser_result_t infix_operator;
  p_space(ctx, &ignore);
  p_literal(ctx, result); if (result->tag == PARSER_SUCCESS) goto success;
  p_indent(ctx, result); if (result->tag == PARSER_SUCCESS) goto success;
 success:
  p_space(ctx, &ignore);
  p_infix_operator(ctx, &infix_operator);
  if (infix_operator.tag == PARSER_SUCCESS) {
    parser_result_t right;
    p_expr(ctx, &right);
    if (result->tag == PARSER_SUCCESS) {
      expr_t *expr = (expr_t*)malloc(sizeof(expr_t));
      expr->tag = EXP_INFIX;
      expr->infix.operator[0] = (intptr_t)infix_operator.success.value;
      expr->infix.operator[1] = '\0';
      expr->infix.left = result->success.value;
      expr->infix.right = right.success.value;
      result->success.value = expr;
      return;
    }
  }
  result->tag = PARSER_SUCCESS;
  return;
}

int parser_unittest(){
  char *input01 = "-54_754_765";
  char *input02 = "'sdfjsdfjsdn'";
  char *input03 = "_sdfjsdfjsdn_ + 85487547564";
  char *input = input03;
  parser_context_t ctx = {input, strlen(input), 0};
  parser_result_t result;
  p_expr(&ctx, &result);
  inspect(%d, result.tag);
  if (result.tag == PARSER_SUCCESS) {
    expr_t *ast = success_value(result, expr_t *);
    inspect(%d, ast->tag);
    if (ast->tag == EXP_LITERAL) {
      inspect(%d, ast->literal.tag);
      if (ast->literal.tag == LITERAL_NUMBER) {
        inspect(%d, ast->literal.number.value);
      }
      if (ast->literal.tag == LITERAL_STRING) {
        inspect(%s, ast->literal.string.utf8);
      }
    }
    if (ast->tag == EXP_IDENT) {
      inspect(%s, ast->ident.utf8);
    }
    if (ast->tag == EXP_INFIX) {
      inspect(%s, ast->infix.operator);
    }
  }
}
