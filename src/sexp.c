#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "main.h"

union sexp_t;

typedef struct {
  unsigned char tt;
} nil_t;

typedef enum {
  TYPE_NIL,
  TYPE_SYMBOL,
  TYPE_STRING,
  TYPE_INTEGER,
  TYPE_PAIR
} sexp_tag_t;

typedef struct {
  unsigned char tt;
  const char *name;
} symbol_t;

typedef struct {
  unsigned char tt;
  const char *str;
} string_t;

typedef struct {
  unsigned char tt;
  int value;
} integer_t;

typedef struct {
  unsigned char tt;
  union sexp_t *car;
  union sexp_t *cdr;
} pair_t;

typedef union sexp_t {
  unsigned char tt;
  symbol_t symbol;
  string_t string;
  integer_t integer;
  pair_t pair;
} sexp_t;

void destroy_sexp(sexp_t *);
void print_sexp(sexp_t *);

sexp_t *create_symbol(const char *name) {
  symbol_t *symbol = (symbol_t *)malloc(sizeof(symbol_t));
  if (!symbol) exit(-1);

  symbol->tt = TYPE_SYMBOL;
  symbol->name = strdup(name);

  return (sexp_t *)symbol;
}

void destroy_symbol(symbol_t *symbol) {
  free((char *)symbol->name);
  free(symbol);
}

void print_symbol(symbol_t *symbol) {
  printf("%s", symbol->name);
}

sexp_t *create_string(const char *str) {
  string_t *string = (string_t *)malloc(sizeof(string_t));
  if (!string) exit(-1);

  string->tt = TYPE_STRING;
  string->str = strdup(str);

  return (sexp_t *)string;
}

void destroy_string(string_t *string) {
  free((char *)string->str);
  free(string);
}

void print_string(string_t *string) {
  printf("\"%s\"", string->str);
}

sexp_t *create_integer(const int value){
  integer_t *integer = (integer_t *)malloc(sizeof(integer_t));
  if (!integer) exit(-1);

  integer->tt = TYPE_INTEGER;
  integer->value = value;

  return (sexp_t *)integer;
}

void destroy_integer(integer_t *integer) {
  free(integer);
}

void print_integer(integer_t *integer){
  printf("%d", integer->value);
}

sexp_t *create_nil() {
  nil_t *nil = (nil_t *)malloc(sizeof(nil_t));
  if (!nil) exit(-1);

  nil->tt = TYPE_NIL;

  return (sexp_t *)nil;
}

void destroy_nil(nil_t *nil){
  free(nil);
}

void print_nil(nil_t *nil) {
  printf("()");
}

sexp_t *create_pair(sexp_t *car, sexp_t *cdr){
  pair_t *pair = (pair_t *)malloc(sizeof(pair_t));
  if (!pair) exit(-1);

  pair->tt = TYPE_PAIR;
  pair->car = car;
  pair->cdr = cdr;

  return (sexp_t *)pair;
}

void destroy_pair(pair_t *pair) {
  destroy_sexp(pair->car);
  destroy_sexp(pair->cdr);
  free(pair);
}

void print_pair(pair_t *pair) {
  printf("(");
 begin:
  print_sexp(pair->car);
  switch (pair->cdr->tt) {
  case TYPE_NIL:
    printf(")");
    break;
  case TYPE_PAIR:
    printf(" ");
    pair = &(pair->cdr->pair);
    goto begin;
  default:
    printf(" . ");
    print_sexp(pair->cdr);
    printf(")");
  }
}


void destroy_sexp(sexp_t *sexp) {
  switch (sexp->tt) {
  case TYPE_NIL:
    destroy_nil((nil_t *)sexp);
    return;
  case TYPE_SYMBOL:
    destroy_symbol((symbol_t *)sexp);
    return;
  case TYPE_STRING:
    destroy_string((string_t *)sexp);
    return;
  case TYPE_INTEGER:
    destroy_integer((integer_t *)sexp);
    return;
  case TYPE_PAIR:
    destroy_pair((pair_t *)sexp);
    return;
  default:
    fprintf(stderr, "Invalid S-expression.\n");
    exit(-1);
  }
}

void print_sexp(sexp_t *sexp) {
  switch (sexp->tt) {
  case TYPE_NIL:
    print_nil((nil_t *)sexp);
    return;
  case TYPE_SYMBOL:
    print_symbol((symbol_t *)sexp);
    return;
  case TYPE_STRING:
    print_string((string_t *)sexp);
    return;
  case TYPE_INTEGER:
    print_integer((integer_t *)sexp);
    return;
  case TYPE_PAIR:
    print_pair((pair_t *)sexp);
    return;
  default:
    fprintf(stderr, "Invalid S-expression.\n");
    exit(-1);
  }
}

bool is_invalid_character(char c) {
  return false;
}

bool is_whitespace(char c) {
  return c == '\t' || c == '\n' || c == '\r' || c == ' ';
}

bool is_terminating(char c) {
  return c == '"' || c == '\'' || c == '(' || c == ')'
    || c == ',' || c == ';'  || c == '`';
}

bool is_non_terminating(char c) {
  return c == '#';
}

bool is_macro_character(char c) {
  return is_terminating(c) || is_non_terminating(c);
}

bool is_single_escape(char c) {
  return c == '\\';
}

bool is_multi_escape(char c) {
  return c == '|';
}

void nreverse(sexp_t **psexp) {
  sexp_t *prev = create_nil();
  sexp_t *current = *psexp;
  sexp_t *next;

  while(!(current->tt == TYPE_NIL)) {
    if (!(current->tt == TYPE_PAIR)) {
      fprintf(stderr, "Not list.");
      exit(-1);
    }
    next = ((pair_t *)current)->cdr;
    ((pair_t *)current)->cdr = prev;
    prev = current;
    current = next;
  }
  destroy_sexp(current);
  *psexp = prev;
}

#define ERROR(_msg) do {                       \
    fprintf(stderr, "%s: %d\n", _msg, __LINE__); \
    exit(EXIT_FAILURE);                         \
  } while(0)

#define END_OF_FILE         ERROR("End of file.")
#define NOT_IMPLEMENTED     ERROR("Not implemented.")
#define READER_ERROR        ERROR("Reader error.")
#define UNMATCHED_CLOSE_PARENTHESIS ERROR("Unmatched close parenthesis.")
#define MUST_NOT_BE_REACHED ERROR("Must not be reached.")

sexp_t *read(FILE *);

sexp_t *read_string(FILE *fp, char c) {
#define ADD_BUF(_buf, _i, _c) do {                              \
    _buf[_i++] = _c;                                            \
    if (_i == 256) {                                            \
      fprintf(stderr, "Too long string: %s...\n", _buf);        \
      exit(-1);                                                 \
    }                                                           \
  } while(0)

  char buf[256] = {0};
  int i = 0;

  while ((c = fgetc(fp)) != EOF) {

    if (is_single_escape(c)) {
      if ((c = fgetc(fp)) == EOF)
        END_OF_FILE;
      ADD_BUF(buf, i, c);
      continue;
    }

    if (c == '"')
      return create_string(buf);

    ADD_BUF(buf, i, c);
  }

  END_OF_FILE;

#undef ADD_BUF
}

sexp_t *read_list(FILE *fp, char c) {
  sexp_t *list = create_nil();

  while ((c = fgetc(fp)) != EOF) {

    if (is_whitespace(c))
      continue;

    if (c == ')') {
      nreverse(&list);
      return list;
    }

    if (c == '.')
      NOT_IMPLEMENTED;

    ungetc(c, fp);
    list = create_pair(read(fp), list);
  }

  END_OF_FILE;
}

bool integer(int *result, const char *str) {
  int i = 0;
  int sign = 1;
  *result = 0;
  if (str[i] == '+')
    i++;
  if (str[i] == '-') {
    sign = -1;
    i++;
  }
  if ('0' <= str[i] && str[i] <= '9') {
    *result += str[i] - 0x30;
    i++;
  } else
    return false;
 begin:
  if ('0' <= str[i] && str[i] <= '9') {
    // No overflow check.
    *result *= 10;
    *result += str[i] - 0x30;
    i++;
    goto begin;
  } else if (str[i] == '\0')
    return true;
  else
    return false;
}

sexp_t *read_module(FILE *fp) {
  sexp_t *list = create_nil();
  while (!feof(fp)) {
    sexp_t *mlist = read(fp);
    if (mlist != NULL) list = create_pair(mlist, list);
  }
  nreverse(&list);
  return list;
}

sexp_t *read(FILE *fp) {
#define ADD_BUF(_buf, _i, _c) do {                              \
    _buf[_i++] = _c;                                            \
    if (_i == 256) {                                            \
      fprintf(stderr, "Too long symbol: %s...\n", _buf);        \
      exit(-1);                                                 \
    }                                                           \
  } while(0)

  char buf[256] = {0};
  int i = 0;
  char c;
  int int_value;

 step1:
  if ((c = fgetc(fp)) == EOF)
    return NULL;

 step2:
  if (is_invalid_character(c))
    READER_ERROR;

 step3:
  if (is_whitespace(c))
    goto step1;

 step4:
  if (is_macro_character(c)) {

    if (c == '"')
      return read_string(fp, c);

    if (c == '#')
      NOT_IMPLEMENTED;

    if (c == '\'')
      NOT_IMPLEMENTED;

    if (c == '(')
      return read_list(fp, c);

    if (c == ')')
      UNMATCHED_CLOSE_PARENTHESIS;

    if (c == ',')
      NOT_IMPLEMENTED;

    if (c == ';')
      NOT_IMPLEMENTED;

    if (c == '|')
      NOT_IMPLEMENTED;

    MUST_NOT_BE_REACHED;
  }

 step5:
  if (is_single_escape(c)) {
    if ((c = fgetc(fp)) == EOF)
      END_OF_FILE;
    ADD_BUF(buf, i, c);
    goto step8;
  }

 step6:
  if (is_multi_escape(c))
    NOT_IMPLEMENTED;

 step7:
  // constituent
  ADD_BUF(buf, i, c);
  goto step8;

 step8:
  if ((c = fgetc(fp)) == EOF)
    goto step10;

  if (is_non_terminating(c)) {
    ADD_BUF(buf, i, c);
    goto step8;
  }

  if (is_single_escape(c)) {
    if ((c = fgetc(fp)) == EOF)
      END_OF_FILE;
    ADD_BUF(buf, i, c);
    goto step8;
  }

  if (is_multi_escape(c))
    NOT_IMPLEMENTED;

  if (is_invalid_character(c))
    READER_ERROR;

  if (is_terminating(c)) {
    ungetc(c, fp);
    goto step10;
  }

  if (is_whitespace(c))
    goto step10;

  // constituent
  ADD_BUF(buf, i, c);
  goto step8;

 step9:
  NOT_IMPLEMENTED;

 step10:
  if (integer(&int_value, buf))
    return create_integer(int_value);

  return create_symbol(buf);

#undef ADD_BUF
}

sexp_t *lisp_plus(sexp_t *exp) {
  int result=0;
  if (exp->tt == TYPE_NIL) goto done;
  if (exp->tt == TYPE_INTEGER) {
    result+=exp->integer.value;
    goto done;
  }

  if (exp->tt != TYPE_PAIR) {
    ERROR("plus: expected one or more integers");
  }
  for(sexp_t *iter=exp; iter->tt!=TYPE_NIL; iter=iter->pair.cdr) {
    if (iter->pair.car->tt != TYPE_INTEGER) {
      ERROR("plus: expected one or more integers");
    }
    result+=iter->pair.car->integer.value;
  }
  done:
  return create_integer(result);
}

void for_each_list(sexp_t *exp, bool (*f) (sexp_t **)) {
  for(sexp_t *iter=exp; iter->tt!=TYPE_NIL; iter=iter->pair.cdr) {
    if (f(&(iter->pair.car))) break;
  }
}

sexp_t *eval(sexp_t *exp) {
  sexp_t *eval_pair(pair_t *pair) {
    switch(pair->car->tt) {
    case TYPE_NIL:
      ERROR("Nil is not a function");
    case TYPE_STRING:
      ERROR("String is not a function");
    case TYPE_INTEGER:
      ERROR("integer is not a function");
    case TYPE_SYMBOL:
      for_each_list(pair->cdr, lambda(bool _ (sexp_t **e) {
        sexp_t *e1 = eval(*e);
        if (e1!=*e) destroy_sexp(*e);
        *e=e1;
        return false;
      }));
      if (strcmp(pair->car->symbol.name, "plus")==0) {
        return lisp_plus(pair->cdr);
      }
      fprintf(stderr, "Unknown function: %s\n", pair->car->symbol.name);
      ERROR("Unknown function");
    case TYPE_PAIR:
      ERROR("Cons pair is not a function");
    }
  }

  switch(exp->tt) {
  case TYPE_NIL:
    return exp;
  case TYPE_STRING:
    return exp;
  case TYPE_INTEGER:
    return exp;
  case TYPE_SYMBOL:
    return exp;
  case TYPE_PAIR:
    return eval_pair(&(exp->pair));
  }
}

#undef MUST_NOT_BE_REACHED
#undef UNMATCHED_CLOSE_PARENTHESIS
#undef READER_ERROR
#undef NOT_IMPLEMENTED
#undef END_OF_FILE
#undef ERROR

int test02(int argc, char **argv) {
  if (argc < 2) {
    fputs("Need at least one argument\n", stderr);
    return EXIT_FAILURE;
  }

  FILE *fp = fopen(argv[1], "r");
  sexp_t *sexp = read_module(fp);
  for_each_list(sexp, lambda(bool _(sexp_t **e) {
    sexp_t *result = eval(*e);
    print_sexp(result);
    printf("\n");
    if (*e!=result) destroy_sexp(result);
    return false;
  }));

  destroy_sexp(sexp);
  fclose(fp);

  return 0;
}
