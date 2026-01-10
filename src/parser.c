#include "parser.h"
#include "errors.h"
#include "lexer.h"
#include "symboltable.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Pratt Parsing Operator precedence */
typedef enum {
    PRECEDENCE_ILLEGAL = -1, /* Whenever some thing desired happens */
    PRECEDENCE_NORMAL = 0,   /* Lowest */
    PRECEDENCE_OROP,         /* or */
    PRECEDENCE_ANDOP,        /* and */
    PRECEDENCE_RELOP,        /* > < >= <= */
    PRECEDENCE_ADDSUB,       /* + - */
    PRECEDENCE_MULDIV,       /* * / */
    PRECEDENCE_UNARY,        /* # ! */
} Precedence;

/* Pratt Parser ExprNode function pointers to call based on infix and prefix
 * expression */
typedef struct ExprNode *(*PrefixFn)(TokenStruct token, Precedence precedence);
typedef struct ExprNode *(*InfixFn)(ExprNode *left, TokenStruct token,
                                    Precedence precedence);

/* Types of Statements Available */
typedef struct AssignmentStatement AssignmentStatement;
typedef struct BlockStatement BlockStatement;
typedef struct CallExpression CallExpression;
typedef struct ExpressionList ExpressionList;
typedef struct ExpressionListStatement ExpressionListStatement;
typedef struct ExpressionStatement ExpressionStatement;
typedef struct ForRangeStatement ForRangeStatement;
typedef struct ForStatement ForStatement;
typedef struct IfStatement IfStatement;
typedef struct LocalVariableDeclareStatment LocalVariableDeclareStatment;
typedef struct NameObject NameObject;
typedef struct PackAssignmentListStatement PackAssignmentListStatement;
typedef struct ReturnStatement ReturnStatement;
typedef struct Variable Variable;
typedef struct VariableDeclareListStatement VariableDeclareListStatement;
typedef struct VariableDeclareStatement VariableDeclareStatement;

typedef struct Expression Expression;
typedef struct Block Block;

/* NOTE: There might be extra types out there but these are the types that will
 * remain until the IR
 *
 * Some types such asw GOTO and LABEL are there just for loops and if statements
 * inorder to jump to different statements
 * */
typedef enum {
    STATEMENT_ERROR,
    STATEMENT_VARIABLE_DECLARE,
    STATEMENT_ASSIGNMENT,
    STATEMENT_EXPRESSION,
    STATEMENT_BLOCK,
    STATEMENT_RETURN,
    STATEMENT_BREAK_OR_CONTINUE,
    STATEMENT_GOTO,
    STATEMENT_LABEL,
    STATEMENT_IF,
} StatementType;

typedef struct
{
    StatementType *statement_type;
    union
    {
        /* When all the structs are defined then we are going to sort these out inorder */
        AssignmentStatement *assignment_statement;
        BlockStatement *block_statement;
        CallExpression *call_expression;
        Expression *expression;
        ExpressionList *expression_list;
        ExpressionListStatement *expression_list_statement;
        ExpressionStatement *expression_statement;
        ForRangeStatement *for_range_statement;
        ForStatement *for_statement;
        IfStatement *if_statement;
        LocalVariableDeclareStatment *local_variable_declare_statement;
        PackAssignmentListStatement *pack_assignment_list_statement;
        ReturnStatement *return_statement;
        Variable *variable;
        VariableDeclareListStatement *variable_declare_list_statement;
        VariableDeclareStatement *variable_declare_statement;
    };
} Statement;

struct Parser
{
    Location *location;
    const char *SymTableKey;
    /* To store or find itself inside the SymTable */
    Statement* statement;
    Precedence precedence;
};

struct Location
{
    const char *filename;
    size_t linenumber;
};

/* Function Declaration of statements parsers  */
Statement* parser_make_variable_declaration(Parser *parser);
Statement* parser_make_statement(Parser* parser, Expression* expression);
AssignmentStatement* parser_make_assignment(Parser *parser, Expression* expression, Location* location);
BlockStatement* parser_make_block_statement(Parser* parser, Block* block, Location* location);
ReturnStatement* parser_make_return_statement(Parser* parser, NameObject* function, ExpressionList* expression_list, Location* location);


StatNode *parse_stat();
ExprNode *parse_expr(Precedence precedence);

void parse_assignment_stat(StatNode *stat, TokenStruct token);
void parse_if_else_stat(StatNode *stat, TokenStruct token);
void parse_while_stat(StatNode *stat, TokenStruct token);
void parse_function_call_stat(StatNode *stat, TokenStruct token);

ExprNode *parse_unary_expr(TokenStruct token, Precedence precedence);
ExprNode *parse_constant_expr(TokenStruct token, Precedence precedence);
ExprNode *parse_identifier_expr(TokenStruct token, Precedence precedence);
ExprNode *parse_grouping_expr(TokenStruct token, Precedence precedence);

ExprNode *parse_binary_expr(ExprNode *left, TokenStruct token,
                            Precedence precedence);
ExprNode *parse_function_call_expr(ExprNode *identifer, TokenStruct token,
                                   Precedence precedence);

/* Parse Rules for Pratt expresssion parsing */
typedef struct
{
    PrefixFn prefix;
    InfixFn infix;
    Precedence precedence;
} ParseRule;

ParseRule rules[] = {
    /* [TokenType] = {PrefixFn, InfixFn, precedence}, */

    /* Literal and Grouping Rules */
    [LITERAL_NUMBER] = {parse_constant_expr, NULL, PRECEDENCE_NORMAL},
    [LITERAL_STRING] = {parse_constant_expr, NULL, PRECEDENCE_NORMAL},
    [IDENTIFIER] = {parse_constant_expr, NULL, PRECEDENCE_NORMAL},
    [NIL] = {parse_constant_expr, NULL, PRECEDENCE_NORMAL},
    [TRUE] = {parse_constant_expr, NULL, PRECEDENCE_NORMAL},
    [FALSE] = {parse_constant_expr, NULL, PRECEDENCE_NORMAL},

    /* Arithmetic Rules */
    [ADD] = {NULL, parse_binary_expr, PRECEDENCE_ADDSUB},
    [SUB] = {NULL, parse_binary_expr, PRECEDENCE_ADDSUB},
    [MUL] = {NULL, parse_binary_expr, PRECEDENCE_MULDIV},
    [DIV] = {NULL, parse_binary_expr, PRECEDENCE_MULDIV},

    /* Comparision Rules */
    [EQUAL_EQUAL] = {NULL, parse_binary_expr, PRECEDENCE_RELOP},
    [NOT_EQUAL] = {NULL, parse_binary_expr, PRECEDENCE_RELOP},
    [LESS_T_EQUAL] = {NULL, parse_binary_expr, PRECEDENCE_RELOP},
    [LESS_T] = {NULL, parse_binary_expr, PRECEDENCE_RELOP},
    [GREATER_T_EQUAL] = {NULL, parse_binary_expr, PRECEDENCE_RELOP},
    [GREATER_T] = {NULL, parse_binary_expr, PRECEDENCE_RELOP},

    /* Misc */
    [LPAREN] = {parse_grouping_expr, parse_function_call_expr,
                PRECEDENCE_NORMAL},
};

ParseRule *get_rule(TokenType type)
{
    return &rules[type];
} /* returns either PrefixFn or InfixFn or NULL */

StatNodeList *create_empty_chunk()
{
    StatNodeList *chunk = malloc(sizeof(StatNodeList));
    if (chunk == NULL) {
        E(fprintf(stderr, "Fatal Error: Memory for chunk not allocated"));
        exit(1);
    }
    chunk->stat = NULL;
    chunk->next = NULL;
    return chunk;
}

StatNodeList *parse_chunk()
{
    StatNodeList *chunk = create_empty_chunk();
    StatNodeList *temp = NULL;
    TokenStruct token;

    while (peek_next_token().type != _EOF) {
        token = peek_next_token();
        if (token.type == ILLEGAL) {
            E(fprintf(stderr, "Syntax Error: %s is unexpected\n",
                      peek_next_token().literal));
            exit(1);
        } else if (token.type == END || token.type == ELSEIF ||
                   token.type == ELSE) {
            break;
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
    }
    return chunk;
}

StatNode *parse_stat()
{
    /**
     * parse_stat checks if the starting symbol of statment is Identifer or not.
     *
     * Case IDENTIFER:
     *      Check if there is equal after it. If yes then parse as
     *      assignment_stat.
     * Case LOCAL:
     *      Check if there is some local variable. if yes then evaluate it
     *      same as IDENTIFER
     *
     * @return StatNode
     **/
    TokenStruct token = consume_token();
    StatNode *stat = malloc(sizeof(StatNode));

    switch (token.type) {
    case IDENTIFIER: {
        if (peek_next_token().type == EQUAL) {
            parse_assignment_stat(stat, token);
        } else if (peek_next_token().type == LPAREN) {
            parse_function_call_stat(stat, token);
        } else {
            E(fprintf(stderr,
                      "Syntax Error: Expected '=' or functioncall after "
                      "Identifer '%s'\n",
                      token.literal));
            exit(1);
        }
    } break;
    case LOCAL: {
        if (peek_next_token().type == IDENTIFIER) {
            token = consume_token(); /* IDENTIFIER */
            if (peek_next_token().type == EQUAL) {
                parse_assignment_stat(stat, token);
            } else if (peek_next_token().type == LPAREN) {
                parse_function_call_stat(stat, token);
            } else {
                E(fprintf(stderr,
                          "Syntax Error: Expected '=' or functioncall after "
                          "Identifer '%s'\n",
                          token.literal));
                exit(1);
            }
        }
    } break;
    case IF: {
        parse_if_else_stat(stat, token);
    } break;
    case WHILE: {
        parse_while_stat(stat, token);
    } break;
    default:
        E(fprintf(stderr, "Syntax Error: Statment should not start with %s\n",
                  token.literal));
        exit(1);
    };
    return stat;
}

void parse_assignment_stat(StatNode *stat, TokenStruct token)
{
    /**
     * parse_assignment_stat assigns assingment_stat node to this StatNode.
     * assingment_stat node as two children: VarNode and ExprNode.
     **/
    stat->data.assingment_stat.var = malloc(sizeof(VarNode));
    stat->type = AssignmentStat;
    stat->data.assingment_stat.var->name = token.literal;
    consume_token(); /* consume equal */
    stat->data.assingment_stat.expr = parse_expr(0.0);
}

void parse_if_else_stat(StatNode *stat, TokenStruct token)
{
    /**
     * parse_if_else_stat check all the valid conditions in their respective
     * functions, here we just structure the code in 'if else' construct
     **/
    stat->data.if_else_stat.if_branches = malloc(sizeof(IfBlockNode));
    stat->data.if_else_stat.if_branches->condition = parse_expr(0.0);
    stat->type = IfElseStat;
    token = consume_token(); /* Consume then */
    if (token.type == THEN) {
        stat->data.if_else_stat.if_branches->block = parse_chunk();
    } else {
        E(fprintf(
            stderr,
            "Syntax Error: Expected 'then' after if statment but got '%s'\n",
            token.literal));
        exit(1);
    }
    token = consume_token(); /* Either elseif, else or end */
    stat->data.if_else_stat.if_branches->next =
        NULL; /* It is important to keep it NULL cause we can use NULL in
                 traversal */
    while (token.type == ELSEIF) {
        IfBlockNode *elseif = malloc(sizeof(IfBlockNode));
        elseif->condition = parse_expr(0.0);
        token = consume_token(); /* Consume then */
        if (token.type == THEN) {
            elseif->block = parse_chunk();
        } else {
            E(fprintf(stderr,
                      "Syntax Error: Expected 'then' after if statment but got "
                      "'%s'\n",
                      token.literal));
            exit(1);
        }
        stat->data.if_else_stat.if_branches->next = elseif;
        elseif->next = NULL;
        token = consume_token(); /* Either elseif, else or end  */
    }
    if (token.type == ELSE) {
        stat->data.if_else_stat.else_block = parse_chunk();
        token = consume_token(); /* end */
    }
    if (token.type != END) {
        E(fprintf(stderr,
                  "Syntax Error: Expected 'end' but got "
                  "'%s'\n",
                  token.literal));
        exit(1);
    }
}

void parse_while_stat(StatNode *stat, TokenStruct token)
{
    stat->data.while_stat.condition = malloc(sizeof(ExprNode));
    stat->data.while_stat.condition = parse_expr(0.0);
    stat->type = WhileLoopStat;
    token = consume_token(); /* Consume do */
    if (token.type == DO) {
        stat->data.while_stat.while_block = parse_chunk();
    } else {
        E(fprintf(stderr,
                  "Syntax Error: Expected 'do' after expression but got "
                  "'%s'\n",
                  token.literal));
        exit(1);
    }
    token = consume_token();
    if (token.type != END) {
        E(fprintf(stderr,
                  "Syntax Error: Expected 'end' but got "
                  "'%s'\n",
                  token.literal));
        exit(1);
    }
}

void parse_function_call_stat(StatNode *stat, TokenStruct token)
{
    stat->data.functioncall.func = malloc(sizeof(FunctionDefNode));
    stat->data.functioncall.func->funcname = malloc(sizeof(VarNode));
    stat->data.functioncall.func->funcname->name = token.literal;
    stat->type = FunctionCAllStat;
    consume_token(); /* Consume LPAREN */
    token = peek_next_token();
    if (token.type == RPAREN) {
        consume_token(); /* Consume RPAREN */
    }
}

ExprNode *parse_expr(Precedence precedence)
{
    /**
     *
     **/
    TokenStruct token = consume_token(); /* First Token After EQUAL */
    PrefixFn prefix_rule = get_rule(token.type)->prefix;
    if (prefix_rule == NULL) {
        E(fprintf(stderr, "Syntax Error: No prefix rule for Token: '%s'\n",
                  token.literal));
        exit(1);
    }

    ExprNode *left_expr = prefix_rule(token, precedence);

    while (precedence < get_rule(peek_next_token().type)->precedence) {
        token = consume_token();
        InfixFn infix_rule = get_rule(token.type)->infix;
        if (infix_rule == NULL) {
            E(fprintf(
                stdout,
                "No infix rule found, Note this could be a normal thing.\n"));
            return left_expr;
        }
        left_expr =
            infix_rule(left_expr, token, get_rule(token.type)->precedence);
    }
    return left_expr;
}

ExprNode *parse_constant_expr(TokenStruct token, Precedence precedence)
{
    ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode));
    switch (token.type) {
    case LITERAL_STRING: {
        expr->type = StringExpr;
        expr->data.literalstring = token.literal;
    } break;
    case LITERAL_NUMBER: {
        char *end_ptr;
        expr->type = NumberExpr;
        expr->data.literalnumber = strtod(token.literal, &end_ptr);
    } break;
    case IDENTIFIER: {
        expr->data.var = malloc(sizeof(VarNode));
        expr->type = VariableExpr;
        expr->data.var->name = token.literal;
    } break;
    case NIL: {
        expr->type = NilExpr;
        expr->data.isnil = 1;
    } break;
    case TRUE: {
        expr->type = BooleanExpr;
        expr->data.boolean = 1;
    } break;
    case FALSE: {
        expr->type = BooleanExpr;
        expr->data.boolean = 0;
    } break;
    default:
        E(fprintf(stderr,
                  "Syntax Error: Expected a number, boolean, nil, identifer or "
                  "string but got '%s' with precedence '%d'\n",
                  token.literal, precedence));
        exit(1);
    }

    return expr;
}

ExprNode *parse_grouping_expr(TokenStruct token, Precedence precedence)
{
    ExprNode *expr = parse_expr(PRECEDENCE_NORMAL);
    token = consume_token();
    if (token.type != RPAREN) {
        E(fprintf(stderr,
                  "Syntax Error: Expected ')' for '(' but got '%s', with "
                  "precedence '%d'",
                  token.literal, precedence));
        exit(1);
    }
    return expr;
}

ExprNode *parse_binary_expr(ExprNode *left, TokenStruct token,
                            Precedence precedence)
{
    ExprNode *expr = malloc(sizeof(ExprNode));
    expr->type = BinaryExpr;
    expr->data.binary_expr.left = left;
    expr->data.binary_expr.op = *token.literal;
    expr->data.binary_expr.right = parse_expr(precedence);
    return expr;
}

ExprNode *parse_function_call_expr(ExprNode *identifier, TokenStruct token,
                                   Precedence precedence)
{
    consume_token();
    if (token.type == LPAREN) {
        return NULL;
    }
    ExprNode *expr = malloc(sizeof(ExprNode));
    expr->data.functioncall.funcname = malloc(sizeof(VarNode));
    expr->type = FunctionCallExpr;
    expr->data.functioncall.funcname->name = identifier->data.var->name;
    token = consume_token(); /* Consume RPAREN */
    if (token.type != RPAREN) {
        E(fprintf(stderr,
                  "Syntax Error: Expected ')' after '(' in a function call "
                  "but got '%s' with precedence '%d'\n",
                  token.literal, precedence));
        exit(1);
    }
    return expr;
}

#ifdef DEBUG_LUA

/* Traverse Tree */
void traverse_stat_node_list(StatNodeList *chunk);
void traverse_stat_node(StatNode *stat);
void traverse_expr(ExprNode *expr);
void traverse_if_else_stat(IfBlockNode *if_branches);
void traverse_functioncall_node(FunctionDefNode *func);

void traverse_stat_node_list(StatNodeList *chunk)
{
    StatNodeList *temp = chunk;
    while (temp != NULL) {
        if (temp->stat == NULL) {
            D(fprintf(stdout, "temp->stat->NULL\n"));
            break;
        } else {
            // Branch from where
            traverse_stat_node(temp->stat);
        }
        temp = temp->next;
    }
}

void traverse_stat_node(StatNode *stat)
{
    /* Check if any node is non-empty */
    if (stat->type == AssignmentStat) {
        printf("stat->data.assingment_stat.var->name->%s\n",
               stat->data.assingment_stat.var->name);
        printf("stat->data.assingment_stat.var->expr->");
        assert(stat->data.assingment_stat.expr != NULL);
        traverse_expr(stat->data.assingment_stat.expr);

    } else if (stat->type == IfElseStat) {
        traverse_if_else_stat(stat->data.if_else_stat.if_branches);
        printf("stat->data.if_else_stat.else_block->\n");
        traverse_stat_node_list(stat->data.if_else_stat.else_block);

    } else if (stat->type == WhileLoopStat) {
        traverse_expr(stat->data.while_stat.condition);
        traverse_stat_node_list(stat->data.while_stat.while_block);

    } else if (stat->type == FunctionCAllStat) {
        traverse_functioncall_node(stat->data.functioncall.func);

    } else {
        D(fprintf(stdout, "stat->NULL\n"));
        return;
    }
}

void traverse_expr(ExprNode *expr)
{
    switch (expr->type) {
    case NumberExpr:
        printf("data.literalnumber = '%F'\n", expr->data.literalnumber);
        break;
    case StringExpr:
        printf("data.literalstring = '%s'\n", expr->data.literalstring);
        break;
    case BooleanExpr:
        printf("data.boolean = '%d'\n", expr->data.boolean);
        break;
    case VariableExpr:
        printf("data.var->name = '%s'\n", expr->data.var->name);
        break;
    case NilExpr:
        printf("data.isnil = '%d'\n", expr->data.isnil);
        break;
    case FunctionCallExpr:
        printf("data.functioncall.funcname->name = '%s'\n",
               expr->data.functioncall.funcname->name);
        break;
    default: {
        if (expr->type == BinaryExpr) {
            printf("data.binary_expr->left->");
            traverse_expr(expr->data.binary_expr.left);
            printf("data.binary_expr.op = '%c'\n", expr->data.binary_expr.op);
            printf("data.binary_expr->right->");
            traverse_expr(expr->data.binary_expr.right);
        }
    } break;
    };
}

void traverse_functioncall_node(FunctionDefNode *func)
{
    printf("func->funcname->name = '%s'\n", func->funcname->name);
    traverse_stat_node_list(func->body);
}

void traverse_if_else_stat(IfBlockNode *if_branches)
{
    printf("%p\n", if_branches);
}

#endif
