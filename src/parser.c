#include "parser.h"
#include "errors.h"
#include "lexer.h"

#include <stdlib.h>

/* Helper Functions Forward Declaration */

// For StatNodeList
StatNodeList *parse_chunk();
StatNodeList *parse_stat_list();

// For StatNode
StatNode *parse_stat();
// void parse_return_stat(StatNode *stat);
void parse_assingment_stat(StatNode *stat);
void parse_localvarlist_stat(StatNode *stat);

VarNodeList *parse_var_list();
VarNode *parse_var();

ExprNode *parse_expr();
ExprNodeList *parse_expr_list();
ExprNode *parse_unary_expr();
ExprNode *parse_binary_expr();
/*
void parse_funccall_stat(StatNode *stat);
void parse_dolike_block_stat(StatNode *stat);
void parse_if_else_stat(StatNode *stat);
void parse_iteratorfor_stat(StatNode *stat);
void parse_unpackfor_stat(StatNode *stat);
void parse_functiondef_stat(StatNode *stat);
void parse_localfunctiondef_stat(StatNode *stat);

// For Function
FuncCall *parse_function_call();
FuncBody *parse_function_body();

// For Field
Field *parse_field();
FieldList *parse_field_list();

// For Var

// For Expr

ExprNode* parse_table_construction_expr();
ExprNode* parse_function_declaration_expr();
ExprNode* parse_var_expr();
ExprNode* parse_string_expr();
ExprNode* parse_number_expr();
There is a chance that we might need them but for now lets leave them.
*/

StatNodeList *create_empty_chunk() {
  StatNodeList *chunk = (StatNodeList *)malloc(sizeof(StatNodeList *));
  if (chunk == NULL) {
    E(fprintf(stderr, "Fatal Error: Memory for chunk not allocated"));
  }
  chunk->stat = NULL;
  chunk->next = NULL;
  return chunk;
}

StatNodeList *parse_chunk() {
  StatNodeList *chunk = create_empty_chunk();
  StatNodeList *temp = NULL;
  TokenStruct token = get_next_token();
  while (1) {
    if (token.type == _EOF || token.type == END || token.type == RETURN ||
        token.type == UNTIL || token.type == ELSEIF || token.type == ELSE) {
      break;
    }
    if (token.type == ILLEGAL) {
      E(fprintf(stderr, "Syntax Error: %s is unexpected", token.literal));
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
  }
  return chunk;
}

StatNode *parse_stat() {
  StatNode *stat = malloc(sizeof(StatNode));
  TokenStruct token = get_next_token();
  switch (token.type) {
    /*
    case LOCAL:
      token = get_next_token();
      if (token.type == FUNCTION) {
        parse_localfunctiondef_stat(stat);
      } else if (token.type == IDENTIFIER) {
        parse_localvarlist_stat(stat);
      }
      break;

    case FUNCTION:
    parse_functiondef_stat(stat);
    break;

    case DO:
    stat->type = DoBlockStat;
    parse_dolike_block_stat(stat);
    break;

    case WHILE:
    stat->type = WhileBlockStat;
    parse_dolike_block_stat(stat);
    break;
    case REPEAT:
    stat->type = RepeatBlockStat;
    parse_dolike_block_stat(stat);
    break;
    case IF:
    parse_if_else_stat(stat);
    break;
    */

  case IDENTIFIER:
    token = get_next_token();
    // if (token.type == LBRACE) {
    //  parse_funccall_stat(stat);
    // } else if (token.type == EQUAL) {
    parse_assingment_stat(stat);
    //}
    break;
    /*
case RETURN:
parse_return_stat(stat);
*/
  default:
    break;
  }
  return stat;
}
/*
void parse_return_stat(StatNode *stat) {
  stat->type = ReturnStat;
  stat->data.return_stat = parse_expr_list();
}

void parse_assingment_stat(StatNode *stat) {
  stat->type = AssignmentStat;
  stat->data.assingment_stat.varlist = parse_var_list();
  stat->data.assingment_stat.exprlist = parse_expr_list();
}

void parse_funccall_stat(StatNode *stat) {
  stat->type = FuncCallStat;
  stat->data.funccall_stat = parse_function_call();
}

void parse_dolike_block_stat(StatNode *stat) {
  TokenStruct token;
  if (stat->type == DoBlockStat) {
    stat->data.dolike_block_stat.block = parse_chunk();
  } else if (stat->type == WhileBlockStat) {
    if (token.type == DO) {
      stat->data.dolike_block_stat.condition = parse_expr();
      stat->data.dolike_block_stat.block = parse_chunk();
    } else {
      E(fprintf(stderr, "Expected do after the block but got %s",
                token.literal));
    }
  } else if (stat->type == RepeatBlockStat) {
    stat->data.dolike_block_stat.block = parse_chunk();
    stat->data.dolike_block_stat.condition = parse_expr();
  } else {
    E(fprintf(stderr, "Expected block but get %s", token.literal));
  }
}

void parse_if_else_stat(StatNode *stat) {}
void parse_iteratorfor_stat(StatNode *stat) {}
void parse_unpackfor_stat(StatNode *stat) {}

void parse_functiondef_stat(StatNode *stat) {
  TokenStruct token = get_next_token();
  stat->type = FunctionDefStat;
  if (token.type == IDENTIFIER) {
    stat->data.functiondef_stat.base_name = token.literal;
  }
  token = get_next_token();
  if (token.type == LPAREN) {
    stat->data.functiondef_stat.field_names = parse_expr_list();
  }
  stat->data.functiondef_stat.body = parse_function_body();
}

FuncBody *parse_function_body() {
  FuncBody *body = malloc(sizeof(FuncBody));
  return body;
}

void parse_localfunctiondef_stat(StatNode *stat) {
  TokenStruct token = get_next_token();
}
void parse_localvarlist_stat(StatNode *stat) {}

FuncCall *parse_function_call() {
  FuncCall *call = malloc(sizeof(FuncBody));
  return call;
}
*/
ExprNode *parse_expr() {
  ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode *));
  TokenStruct token = get_next_token();
  if (token.type == LITERAL_NUMBER) {
    expr->type = NumberExpr;
    expr->data.number_expr = (double)atoi(token.literal);
  }
  return expr;
}

ExprNodeList *parse_expr_list() {
  ExprNodeList *exprlist = (ExprNodeList *)malloc(sizeof(ExprNodeList *));
  exprlist->curr_expr = parse_expr();
  return exprlist;
}

VarNodeList *parse_var_list() {
  VarNodeList *varlist = (VarNodeList *)malloc(sizeof(VarNodeList *));
  varlist->var = parse_var();
  return varlist;
}

VarNode *parse_var() {
  VarNode *var = (VarNode *)malloc(sizeof(VarNode *));
  TokenStruct token = get_next_token();
  var->type = NameVar;
  var->data.name = token.literal;
  return var;
}

/* Traversal Functions */
/* Helper functions */
void traverse_stat(StatNode *node);
void traverse_expr(ExprNode *node);
void traverse_var(VarNode *node);
void traverse_func_body(FuncBody *body);
void traverse_func_call(FuncCall *call);
void traverse_field(Field *field);
void traverse_name_list(NameList *list);
void traverse_var_list(VarNodeList *list);
void traverse_expr_list(ExprNodeList *explist);
void traverse_stat_list(StatNodeList *statlist);

void traverse_ast_chunk(StatNodeList *list) {
  if (list == NULL) {
    return;
  }

  StatNodeList *current = list;
  printf("\n--- TRAVERSAL START: CHUNK (Block of Statements) ---\n");

  while (current != NULL) {
    traverse_stat(current->stat);
    current = current->next;
  }

  printf("\n--- TRAVERSAL END: CHUNK ---\n");
}

void traverse_stat_list(StatNodeList *list) {
  if (list == NULL)
    return;
  StatNodeList *current = list;
  while (current != NULL) {
    traverse_stat(current->stat);
    current = current->next;
  }
}

void traverse_stat(StatNode *node) {
  if (node == NULL)
    return;

  printf("STAT: ");
  switch (node->type) {
  case AssignmentStat:
    printf("Assignment\n");
    traverse_var_list(node->data.assingment_stat.varlist);
    traverse_expr_list(node->data.assingment_stat.exprlist);
    break;

  case FuncCallStat:
    printf("Function Call\n");
    traverse_func_call(node->data.funccall_stat);
    break;

  case DoBlockStat:
    printf("Do Block\n");
    traverse_stat_list(node->data.dolike_block_stat.block);
    break;

  case WhileBlockStat:
  case RepeatBlockStat:
    printf("%s Block\n", node->type == WhileBlockStat ? "While" : "Repeat");
    traverse_expr(node->data.dolike_block_stat.condition);
    traverse_stat_list(node->data.dolike_block_stat.block);
    break;

  case IfBlockStat:
    printf("If Block\n");
    traverse_expr(node->data.if_else_stat.condition);
    traverse_stat_list(node->data.if_else_stat.block);
    traverse_stat(node->data.if_else_stat.next_clause);
    break;

  case FunctionDefStat:
  case LocalFunctionDefStat:
    printf("%s Function Definition\n",
           node->type == FunctionDefStat ? "Global" : "Local");
    traverse_func_body(node->type == FunctionDefStat
                           ? node->data.functiondef_stat.body
                           : node->data.localfunctiondef_stat.body);
    break;

  case BreakStat:
    printf("Break Statement\n");
    break;

  case EmptyStat:
    printf("Empty Statement\n");
    break;

  default:
    fprintf(stderr, "ERROR: Unknown StatNode type: %d\n", node->type);
    break;
  }
}

void traverse_expr_list(ExprNodeList *list) {
  if (list == NULL)
    return;
  ExprNodeList *current = list;
  while (current != NULL) {
    traverse_expr(current->curr_expr);
    current = current->next;
  }
}

void traverse_expr(ExprNode *node) {
  if (node == NULL)
    return;

  printf("  EXPR: ");
  switch (node->type) {
  case NumberExpr:
    printf("Number (%f)\n", node->data.number_expr);
    break;
  case StringExpr:
    printf("String (\"%s\")\n", node->data.string_expr);
    break;
  case BinaryExpr:
    printf("Binary Op (%c)\n", node->data.binary_expr.op);
    traverse_expr(node->data.binary_expr.left);
    traverse_expr(node->data.binary_expr.right);
    break;
  case UnaryExpr:
    printf("Unary Op (%c)\n", node->data.unary_expr.op);
    traverse_expr(node->data.unary_expr.left);
    break;
  case FuncDecExpr:
    printf("Function Declaration\n");
    traverse_func_body(node->data.funcdec_expr);
    break;
  case PrefixExpr:
    printf("Prefix Expression (usually a Var or Call)\n");
    break;
  default:
    printf("Simple Expr (%d)\n", node->type);
    break;
  }
}

void traverse_var_list(VarNodeList *list) {
  if (list == NULL)
    return;
  VarNodeList *current = list;
  while (current != NULL) {
    traverse_var(current->var);
    current = current->next;
  }
}

void traverse_var(VarNode *node) {
  if (node == NULL)
    return;

  printf("    VAR: ");
  switch (node->type) {
  case NameVar:
    printf("Name (%s)\n", node->data.name);
    break;
  case IndexVar:
    printf("Index Access\n");
    traverse_expr(node->data.complex_var.prefix);
    traverse_expr(node->data.complex_var.key);
    break;
  case MemberVar:
    printf("Member Access\n");
    traverse_expr(node->data.complex_var.prefix);
    break;
  }
}

void traverse_func_body(FuncBody *body) {
  if (body == NULL)
    return;
  printf("  FUNC BODY: Parameters and Block\n");
  traverse_stat_list(body->block);
}

void traverse_func_call(FuncCall *call) {
  if (call == NULL)
    return;
  printf("  FUNC CALL: %s\n", call->type == normal_call ? "Normal" : "Member");
  traverse_expr(call->prefix);
  traverse_expr_list(call->arguments);
}
