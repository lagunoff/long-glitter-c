#pragma once

#define success_value(result, type) ((type)result.success.value)

struct expr_t;

typedef struct {
  int value;
} literal_number_t;

typedef struct {
  char *utf8;
  int   len;
} literal_string_t;

typedef struct {
  char           key[16];
  struct expr_t *value;
} record_field_t;

typedef struct {
  record_field_t *fields;
  int             len;
} literal_record_t;

typedef struct {
  struct expr_t *elems;
  int            len;
} literal_array_t;

typedef struct {
  enum {
    Literal_String,
    Literal_Number,
    Literal_Array,
    Literal_Record,
  } tag;
  union {
    literal_number_t number;
    literal_string_t string;
    literal_array_t  array;
    literal_record_t record;
  };
} literal_t;

typedef struct {
  char *utf8;
  int   len;
} ident_t;

typedef struct {
  char           operator[16];
  struct expr_t *left;
  struct expr_t *right;
} infix_t;

struct expr_t {
  enum {
    Expr_Literal,
    Expr_Ident,
    Expr_Infix,
    Expr_Apply,
  } tag;
  union {
    literal_t literal;
    ident_t   ident;
    infix_t   infix;
  };
};
typedef struct expr_t expr_t;

typedef struct {
  char *input;
  int   len;
  int   offset;
} parser_context_t;

typedef struct {
  void *value;
} parser_success_t;

typedef struct {
  int   lineno;
  int   colno;
  char *message;
} parser_failure_t;

typedef struct {
  enum {
    Parser_Success,
    Parser_Failure,
  } tag;
  union {
    parser_success_t success;
    parser_failure_t failure;
  };
} parser_result_t;

typedef void (*parser_t)(parser_context_t *, parser_result_t *);

void p_number(parser_context_t *ctx, parser_result_t *result);
void p_string(parser_context_t *ctx, parser_result_t *result);
void p_literal(parser_context_t *ctx, parser_result_t *result);
void p_space(parser_context_t *ctx, parser_result_t *result);
void p_expr(parser_context_t *ctx, parser_result_t *result);
void p_indent(parser_context_t *ctx, parser_result_t *result);
