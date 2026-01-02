#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef enum {
    PREC_NIL,
    PREC_COMP_EQUAL,
    PREC_ADDSUB,
    PREC_MULDIV,
    PREC_UNARY,
    PREC_FUNC_CALL,
} OpPrecedence;

/* Forward Declarations */
typedef struct StatNodeList StatNodeList; /* Chunk or Block */
typedef struct StatNode StatNode;
typedef struct ExprNode ExprNode;
typedef struct VarNode VarNode;
typedef struct IfBlockNode IfBlockNode;
typedef struct FunctionDefNode FunctionDefNode;

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

        struct
        {
            VarNode *funcname;
        } functioncall;

    } data;
};

struct VarNode
{
    char *name;
};

struct VarNodeList
{
    VarNode *var;
    VarNode *next;
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
        struct
        {
            IfBlockNode *if_branches;
            StatNodeList *else_block;
        } if_else_stat;
        struct
        {
            ExprNode *condition;
            StatNodeList *while_block;
        } while_stat;
        struct
        {
            FunctionDefNode *func;
        } functioncall;
    } data;
};

struct IfBlockNode
{
    ExprNode *condition;
    StatNodeList *block;
    IfBlockNode *next;
};

struct StatNodeList
{
    StatNode *stat;
    StatNodeList *next;
};

struct FunctionDefNode
{
    VarNode *funcname;
    /* Added parameter list in future */
    StatNodeList *body;
};

typedef struct
{
    PrefixFn prefix;
    InfixFn infix;
    OpPrecedence precedence;
} ParseRule;

StatNodeList *parse_chunk(); /* This is the main node that should be exposed */

#endif // PARSER_H
