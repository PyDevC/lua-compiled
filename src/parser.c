#include "parser.h"
#include "errors.h"
#include "lexer.h"

#include <stdlib.h>
#include <string.h>

/* Helper Functions Forward Declaration */
StatNodeList *parse_chunk();

/* Statment functions */
StatNode *parse_stat();
void parse_assignment_stat(StatNode *stat, TokenStruct token);
/* a = 10 */

/* Expr functions */
ExprNode *parse_expr(ExprNode *expr, TokenType type, OpPrecedence precedence);
/* Parse Expression and use get_rule to tell which rult to apply */

ExprNode *parse_binary_expr(ExprNode *left, TokenType type,
                            OpPrecedence precedence);
/* a = a + b */
ExprNode *parse_unary_expr(TokenStruct token, OpPrecedence precedence);
/* a = -10 */
ExprNode *parse_constant_expr(TokenStruct token, OpPrecedence precedence);
/* a = 10 or a = nil or a = "10" or a = true */
ExprNode *parse_identifier_expr(TokenStruct token, OpPrecedence precedence);
/* a = b */

// TODO: ExprNode *parse_grouping_expr(TokenStruct token, OpPrecedence
// precedence);
/* (a + b) + c */

/* Parsing Rules */
ParseRule rules[] = {
    /* Literal and Grouping Rules */
    /* [TokenType] = {PrefixFn, InfixFn, precedence}, */

    [LITERAL_NUMBER] = {parse_constant_expr, NULL, PREC_NIL},
    [LITERAL_STRING] = {parse_constant_expr, NULL, PREC_NIL},
    [IDENTIFIER] = {parse_constant_expr, NULL, PREC_NIL},
    [NIL] = {parse_constant_expr, NULL, PREC_NIL},
    [TRUE] = {parse_constant_expr, NULL, PREC_NIL},
    [FALSE] = {parse_constant_expr, NULL, PREC_NIL},

    /* Arithmetic Rules */
    [ADD] = {NULL, parse_binary_expr, PREC_ADDSUB},
    [SUB] = {NULL, parse_binary_expr, PREC_ADDSUB},
    [MUL] = {NULL, parse_binary_expr, PREC_MULDIV},
    [DIV] = {NULL, parse_binary_expr, PREC_MULDIV},
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

    while (peek_next_token().type != _EOF) {
        if (peek_next_token().type == ILLEGAL) {
            E(fprintf(stderr, "Syntax Error: %s is unexpected\n",
                      peek_next_token().literal));
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
    /**
     * parse_stat checks if the starting symbol of statment is Identifer or not.
     *
     * Case IDENTIFER:
     *      Check if there is equal after it. If yes then parse as
     * assignment_stat.
     *
     * @return StatNode
     **/
    TokenStruct token = consume_token();
    StatNode *stat = malloc(sizeof(StatNode));

    switch (token.type) {
    case IDENTIFIER: {
        if (peek_next_token().type == EQUAL) {
            parse_assignment_stat(stat, token);
        } else {
            E(fprintf(stderr,
                      "Syntax Error: Expected '=' after Identifer '%s'\n",
                      token.literal));
            exit(1);
        }
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
    stat->data.assingment_stat.var->name = token.literal;
    stat->data.assingment_stat.expr = malloc(sizeof(ExprNode));
    stat->data.assingment_stat.expr =
        parse_expr(stat->data.assingment_stat.expr, token.type, 0.0);
}

ExprNode *parse_expr(ExprNode *expr, TokenType type, OpPrecedence precedence)
{
    /**
     *
     **/
    TokenStruct token = peek_next_token(); /* First Token After EQUAL */
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
            infix_rule(left_expr, token.type, get_rule(token.type)->precedence);
    }
    return left_expr;
}

ExprNode *parse_constant_expr(TokenStruct token, OpPrecedence precedence)
{
    ExprNode *expr = (ExprNode *)malloc(sizeof(ExprNode));
    switch (token.type) {
    case LITERAL_STRING: {
        expr->data.literalstring = token.literal;
    } break;
    case LITERAL_NUMBER: {
        char *end_ptr;
        expr->data.literalnumber = strtod(token.literal, &end_ptr);
    } break;
    case IDENTIFIER: {
        expr->data.var = malloc(sizeof(VarNode));
        expr->data.var->name = token.literal;
    } break;
    case NIL: {
        expr->data.isnil = 1;
    } break;
    case TRUE: {
        expr->data.boolean = 1;
    } break;
    case FALSE: {
        expr->data.boolean = 0;
    } break;
    default:
        E(fprintf(stderr,
                  "Syntax Error: Expected a number, boolean, nil, identifer or "
                  "string but got '%s'\n",
                  token.literal));
        exit(1);
    }

    fprintf(stdout, "Just for test we have precedence: %d\n", precedence);
    return expr;
}

ExprNode *parse_binaryop_expr(ExprNode *left, TokenType type,
                              OpPrecedence precedence)
{
    ExprNode *expr = malloc(sizeof(ExprNode));
    ExprNode *right = malloc(sizeof(ExprNode));
    expr->data.binary_expr.left = left;
    expr->data.binary_expr.op = type;
    expr->data.binary_expr.right = parse_expr(right, type, precedence);
    return expr;
}
