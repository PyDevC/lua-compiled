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
void parse_if_else_stat(StatNode *stat, TokenStruct token);
/* if a == 1 then
 *     a = 2
 *  elseif a == 2 then
 *     a = 1
 *  else
 *     a = 0
 *  end
 */

/* Expr functions */
ExprNode *parse_expr(OpPrecedence precedence);
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

    /* Comparision Rules */
    [EQUAL_EQUAL] = {NULL, parse_binary_expr, PREC_COMP_EQUAL},
    [NOT_EQUAL] = {NULL, parse_binary_expr, PREC_COMP_EQUAL},
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
    case LOCAL: {
        if (peek_next_token().type == IDENTIFIER) {
            token = consume_token(); /* IDENTIFIER */
            if (peek_next_token().type == EQUAL) {
                parse_assignment_stat(stat, token);
            }
        }
    } break;
    case IF: {
        parse_if_else_stat(stat, token);
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
    consume_token(); /* consume equal */
    stat->data.assingment_stat.expr = malloc(sizeof(ExprNode));
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

ExprNode *parse_expr(OpPrecedence precedence)
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
                  "string but got '%s' with precedence '%d'\n",
                  token.literal, precedence));
        exit(1);
    }

    return expr;
}

ExprNode *parse_binary_expr(ExprNode *left, TokenType type,
                            OpPrecedence precedence)
{
    ExprNode *expr = malloc(sizeof(ExprNode));
    expr->data.binary_expr.left = left;
    expr->data.binary_expr.op = type;
    expr->data.binary_expr.right = parse_expr(precedence);
    return expr;
}
