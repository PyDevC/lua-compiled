#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef enum { PREC_NIL, PREC_ADDSUB, PREC_MULDIV, PREC_UNARY } OpPrecedence;

/* Forward Declarations */
typedef struct StatNodeList StatNodeList; /* Chunk or Block */
typedef struct StatNode StatNode;
typedef struct ExprNode ExprNode;
typedef struct VarNode VarNode;

/* Function pointers to call the parser_function for desired token */
typedef struct ExprNode *(*PrefixFn)(TokenStruct token,
                                     OpPrecedence precedence);
typedef struct ExprNode *(*InfixFn)(ExprNode *left, TokenType type,
                                    OpPrecedence precedence);

struct ExprNode
{
    union
    {
        double literalnumber; /* Convert litearl string to double before adding
                                 to this Expr */
        char *literalstring;
        short isnil;
        short boolean;
        VarNode *var;

        struct
        {
            TokenType op;
            ExprNode *left;
            ExprNode *right;
        } binary_expr;

        struct
        {
            TokenType op;
            ExprNode *right;
        } unary_expr;

    } data;
};

struct VarNode
{
    char *name;
};

struct StatNode
{
    union
    {
        struct
        {
            VarNode *var;
            ExprNode *expr;
        } assingment_stat;
    } data;
};

struct StatNodeList
{
    StatNode *stat;
    StatNodeList *next;
};

typedef struct
{
    PrefixFn prefix;
    InfixFn infix;
    OpPrecedence precedence;
} ParseRule;

StatNodeList *parse_chunk(); /* This is the main node that should be exposed */

#endif // PARSER_H
