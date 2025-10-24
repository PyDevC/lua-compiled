#include "parser.h"
#include <stdlib.h>

AstNode *make_binary_expr(char *op, AstNode *left, AstNode *right) {
  AstNode *new = (AstNode *)malloc(sizeof(AstNode));
  // Add something to check if the types are correct or not.
  new->type = BinaryExpr;
  new->op.binary_expr.opr = *op;
  new->op.binary_expr.left = left;
  new->op.binary_expr.right = right;
  return new;
}

AstNode *make_unary_expr(char *op, AstNode *right) {
  AstNode *new = (AstNode *)malloc(sizeof(AstNode));
  // Add something to check if the types are correct or not.
  new->type = UnaryExpr;
  new->op.binary_expr.opr = *op;
  new->op.binary_expr.right = right;
  return new;
}

AstNode *make_integer_expr(int value) {
  AstNode *new = (AstNode *)malloc(sizeof(AstNode));
  // Add something to check if the types are correct or not.
  new->type = IntegerExpr;
  new->op.integer_expr = value;
  return new;
}

AstNode *make_string_expr(char *string) {
  AstNode *new = (AstNode *)malloc(sizeof(AstNode));
  // Add something to check if the types are correct or not.
  new->type = StringExpr;
  new->op.string_expr = string;
  return new;
}

AstNode *make_variable_expr(char *name, AstNode *value) {
  AstNode *new = (AstNode *)malloc(sizeof(AstNode));
  // Add something to check if the types are correct or not.
  new->type = VariableExpr;
  new->op.variable_expr.name = name;
  new->op.variable_expr.value = value;
  return new;
}

AstNode *make_call_expr(char *name, AstNodeList *arguments) {
  AstNode *new = (AstNode *)malloc(sizeof(AstNode));
  // Add something to check if the types are correct or not.
  new->type = CallExpr;
  new->op.call_expr.name = name;
  new->op.call_expr.arguments = arguments;
  return new;
}
