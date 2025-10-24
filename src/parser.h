#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <string.h>

typedef struct Expr {
  enum {
    IntegerExpr,
    StringExpr,
    VariableExpr,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
  } type;

  union {
    int integer_expr;
    char *string_expr;
    struct {
      char *name;
      struct Expr *value;
    } variable_expr;
    struct {
      char opr;
      struct Expr *left;
      struct Expr *right;
    } binary_expr;

    struct {
      char opr;
      struct Expr *right;
    } unary_expr;

    struct {
      char *name;
      struct Expr_list *arguments;
    } call_expr;

  } op;
} AstNode;

typedef struct Expr_list {
  AstNode *elem;
  struct Expr_list *next;
} AstNodeList;

AstNode *parse_token(TokenStruct);

#endif // PARSER_H
