#include "parser.h"
#include "errors.h"
#include "lexer.h"

#include <stdlib.h>

static TokenStruct global_token;
/* Helper Functions Forward Declaration */
StatNodeList *parse_chunk();

/* Statment functions */
StatNode *parse_stat();
void parse_var_stat(StatNode *stat);
void parse_localvar_stat(StatNode *stat);

/* Expr functions */
ExprNode *parse_expr();
void parse_binaryop_expr(ExprNode *expr, int binop_type);
void parse_prefix_expr(ExprNode *expr);
void parse_unaryop_expr(ExprNode *expr, int unary_type);

StatNodeList *create_empty_chunk() {
  StatNodeList *chunk = (StatNodeList *)malloc(sizeof(StatNodeList *));
  if (chunk == NULL) {
    E(fprintf(stderr, "Fatal Error: Memory for chunk not allocated"));
    exit(1);
  }
  chunk->stat = NULL;
  chunk->next = NULL;
  return chunk;
}

StatNodeList *parse_chunk() {
  StatNodeList *chunk = create_empty_chunk();
  StatNodeList *temp = NULL;
  global_token = get_next_token();
  TokenStruct token = global_token;
  while (1) {
    if (token.type == _EOF) {
      break;
    }
    if (token.type == ILLEGAL) {
      E(fprintf(stderr, "Syntax Error: %s is unexpected", token.literal));
      exit(1);
    }

    StatNode *stat = parse_stat();
    if (stat != NULL) {
      StatNodeList *new_node = create_empty_chunk();
      new_node->stat = stat;
      new_node->next = NULL;
      if (chunk->stat == NULL) {
        chunk = new_node;
        temp = chunk;
      } else {
        temp->next = new_node;
        temp = new_node;
      }
    }
    token = get_next_token();
    global_token = token;
  }
  return chunk;
}

StatNode *parse_stat() {
  StatNode *stat = malloc(sizeof(StatNode));
  switch (global_token.type) {
  case IDENTIFIER:
    parse_var_stat(stat);
    break;
  case LOCAL:
    parse_localvar_stat(stat);
    break;
  default:
    E(fprintf(stderr, "Syntax Error: Token does not match for the statement %d",
              global_token.type));
    exit(1);
  };
  return stat;
}

void parse_var_stat(StatNode *stat) {
  TokenStruct token = get_next_token();
  stat->type = AssignmentStat;
  stat->data.assingment_stat.var = (VarNode *)malloc(sizeof(VarNode *));
  stat->data.assingment_stat.var->type = NameVar;
  stat->data.assingment_stat.var->name = global_token.literal;
  if (token.type == EQUAL) {
    stat->data.assingment_stat.expr = parse_expr();
  } else {
    global_token = token;
  }
}

void parse_localvar_stat(StatNode *stat) {
  stat->type = LocalVarListDefStat;
  TokenStruct token = get_next_token();
  stat->data.localvarlist_stat.namelist->name = token.literal;
  if (token.type == EQUAL) {
    stat->data.localvarlist_stat.expr = parse_expr();
  } else {
    global_token = token;
  }
}

ExprNode *parse_expr() {
  ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode *));
  TokenStruct token;
  switch (global_token.type) {
  case NIL:
    expr->type = NilExpr;
    break;
  case FALSE:
    expr->type = FalseExpr;
    break;
  case TRUE:
    expr->type = TrueExpr;
    break;
  case LITERAL_NUMBER:
    token = get_next_token();
    switch (token.type) {
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case EQUAL_EQUAL:
    case NOT_EQUAL:
    case AND:
    case OR:
      expr->type = BinaryExpr;
      expr->data.binary_expr.left = parse_expr();
      parse_binaryop_expr(expr, token.type);
      break;
    default:
      expr->type = NumberExpr;
      expr->data.number_expr = (double)atoi(global_token.literal);
      global_token = token;
      break;
    };
    break;
  case LITERAL_STRING:
    expr->type = StringExpr;
    expr->data.string_expr = global_token.literal;
    break;
  case IDENTIFIER:
  case LPAREN:
    expr->type = PrefixExpr;
    parse_prefix_expr(expr);
    break;
  case SUB:
  case NOT:
  case HASH:
    parse_unaryop_expr(expr, global_token.type);
    break;
  default:
    E(fprintf(stderr, "Syntax Error: Token Type should be a one of ExprType"));
    exit(1);
  }
  return expr;
}

void parse_binaryop_expr(ExprNode *expr, int binop_type) {
  switch (binop_type) {
  case ADD:
    expr->data.binary_expr.op = "+";
    break;
  case SUB:
    expr->data.binary_expr.op = "-";
    break;
  case MUL:
    expr->data.binary_expr.op = "*";
    break;
  case DIV:
    expr->data.binary_expr.op = "/";
    break;
  case EQUAL_EQUAL:
    expr->data.binary_expr.op = "==";
    break;
  case NOT_EQUAL:
    expr->data.binary_expr.op = "~=";
    break;
  case AND:
    expr->data.binary_expr.op = "and";
    break;
  case OR:
    expr->data.binary_expr.op = "or";
    break;
  };
  expr->data.binary_expr.right = parse_expr();
}

void parse_unaryop_expr(ExprNode *expr, int unaryop) {
  switch (unaryop) {
  case SUB:
    expr->data.unary_expr.op = "-";
    break;
  case HASH:
    expr->data.unary_expr.op = "#";
    break;
  case NOT:
    expr->data.unary_expr.op = "not";
    break;
  }
  expr->data.unary_expr.right = parse_expr();
}

void parse_prefix_expr(ExprNode *expr) {
  expr->data.prefix_expr_var = (VarNode *)malloc(sizeof(VarNode *));
  expr->data.prefix_expr_var->name = global_token.literal;
  expr->data.prefix_expr_var->type = NameVar;
}
