#include "parser.h"
#include "errors.h"
#include "lexer.h"

#include <stdlib.h>
#include <string.h>

/* Helper Functions Forward Declaration */
StatNodeList *parse_chunk();

/* Statment functions */
StatNode *parse_stat();
void parse_var_stat(StatNode *stat);
void parse_localvar_stat(StatNode *stat);
void parse_assignment_or_function_call(StatNode *stat, TokenStruct token);

/* Expr functions */
ExprNode *parse_expr(ExprNode *expr, TokenType type, OpPrecedence precedence);
ExprNode *parse_binaryop_expr(ExprNode *left, TokenType type,
                              OpPrecedence precedence);
ExprNode *parse_unaryop_expr(TokenStruct token, OpPrecedence precedence);
ExprNode *parse_number_expr(TokenStruct token, OpPrecedence precedence);
ExprNode *parse_string_expr(TokenStruct token, OpPrecedence precedence);
ExprNode *parse_nil_expr(TokenStruct token, OpPrecedence precedence);
ExprNode *parse_boolean_expr(TokenStruct token, OpPrecedence precedence);
ExprNode *parse_grouping_expr(TokenStruct token, OpPrecedence precedence);

/* Parsing Rules */
ParseRule rules[] = {
    /* Literal and Grouping Rules */
    [LITERAL_NUMBER] = {parse_number_expr, NULL, PREC_NIL},
    [LITERAL_STRING] = {parse_string_expr, NULL, PREC_NIL},
    //[NIL] = {parse_string_expr, NULL, PREC_NIL},
    //[TRUE] = {parse_boolean_expr, NULL, PREC_NIL},
    //[FALSE] = {parse_boolean_expr, NULL, PREC_NIL},
    // [LPAREN] = {parse_grouping_expr, parse_expr, PREC_NULL},
    /* TODO: Check if it should be implemented in this way */

    /* Arithmetic Rules */
    [ADD] = {NULL, parse_binaryop_expr, PREC_ADDSUB},
    [SUB] = {NULL, parse_binaryop_expr, PREC_ADDSUB},
    [MUL] = {NULL, parse_binaryop_expr, PREC_MULDIV},
    [DIV] = {NULL, parse_binaryop_expr, PREC_MULDIV},

    /* String Operator Rules */
    //[HASH] = {parse_unaryop_expr, NULL, PREC_UNARY},

    /* Relational Operator Rules */
    //[EQUAL_EQUAL] = {NULL, parse_binaryop_expr, PREC_OR},
    //[NOT_EQUAL] = {NULL, parse_binaryop_expr, PREC_OR},
    //[AND] = {NULL, parse_binaryop_expr, PREC_AND},
    //[OR] = {NULL, parse_binaryop_expr, PREC_OR},
    //[NOT] = {parse_unaryop_expr, NULL, PREC_UNARY},
};

ParseRule *get_rule(TokenType type) { return &rules[type]; }

StatNodeList *create_empty_chunk()
{
    StatNodeList *chunk = (StatNodeList *)malloc(sizeof(StatNodeList *));
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
    TokenStruct token = peek_next_token();

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
    }
    return chunk;
}

StatNode *parse_stat()
{
    TokenStruct token = consume_token();
    // if (global_tracestack.last->error) {
    //     terminate();
    // }
    StatNode *stat = malloc(sizeof(StatNode));
    memset(stat, 0, sizeof(StatNode));
    switch (token.type) {
    case IDENTIFIER:
        parse_assignment_or_function_call(stat, token);
        break;
    // case LOCAL:
    //     parse_localvar_stat(stat);
    //     break;
    default:
        E(fprintf(
            stderr,
            "Syntax Error: Token does not match for the statement %d: %s\n",
            token.type, token.literal));
        exit(1);
    };
    return stat;
}

void parse_assignment_or_function_call(StatNode *stat, TokenStruct token)
{
    TokenStruct advance = peek_next_token();

    switch (advance.type) {
    case EQUAL:
    case COMMA: {
        /* Assignment */
        stat->type = AssignmentStat;
        stat->data.assingment_stat.var = malloc(sizeof(VarNode));
        stat->data.assingment_stat.var->name = token.literal;
        stat->data.assingment_stat.var->type = NameVar;
        /*TODO: Add a while loop here to implement Multiple assingments
         * Check if the next type is comma or equal according to that
         * you can implement what to do if we have equal or comma after a new
         * identifier
         */
        if (advance.type == COMMA) {
            printf("TODO: Multiple assingments not implemented\n");
            exit(1);
            token = consume_token();
            advance = peek_next_token();
            while (1) {
                if (advance.type == IDENTIFIER) {
                    /* Var, Var */
                    token = consume_token();
                } else {
                    terminate();
                }
            }
        } else {
            token = consume_token(); /* token = EQUAL */
            stat->data.assingment_stat.expr =
                parse_expr(NULL, peek_next_token().type, PREC_NULL);
        }
    } break;
    case LPAREN: {
        stat->type = FunctionCallStat;
        printf("TODO: function call to be implemented\n");
        exit(1);
    } break;
    default:
        break;
    };
}

ExprNode *parse_expr(ExprNode *expr, TokenType type, OpPrecedence precedence)
{
    TokenStruct token = consume_token(); /* token = IDENTIFIER | LITERAL_NUMBER
                                            | LITERAL_STRING */
    PrefixFn prefix_rule = get_rule(type)->prefix;
    if (prefix_rule == NULL) {
        E(fprintf(stderr, "No prefix rule found\n"));
        return expr;
    }

    ExprNode *left = prefix_rule(token, precedence);
    while (precedence < get_rule(peek_next_token().type)->precedence) {
        token = peek_next_token();
        ExprNode *left =
            get_rule(token.type)
                ->infix(left, token.type, get_rule(token.type)->precedence);
    }
    return left;
}

ExprNode *parse_number_expr(TokenStruct token, OpPrecedence precedence)
{
    ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode));
    expr->type = NumberExpr;
    char *endptr;
    expr->data.number_expr = strtod(token.literal, &endptr);
    printf("%d\n", precedence);
    return expr;
}

ExprNode *parse_string_expr(TokenStruct token, OpPrecedence precedence)
{
    ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode));
    expr->type = StringExpr;
    expr->data.string_expr = token.literal;
    printf("%d\n", precedence);
    return expr;
}

ExprNode *parse_binaryop_expr(ExprNode *left, TokenType type,
                              OpPrecedence precedence)
{
    ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode));
    expr->type = BinaryExpr;
    expr->data.binary_expr.left = left;
    expr->data.binary_expr.op = type;
    expr->data.binary_expr.right = parse_expr(NULL, type, precedence);
    return expr;
}
