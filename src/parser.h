#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef struct location_t location_t;

typedef enum {
    PREC_NIL,        /* Lowest */
    PREC_LOGOR,      /* or */
    PREC_LOGAND,     /* and */
    PREC_COMP_EQUAL, /* > < >= <= */
    PREC_ADDSUB,     /* + - */
    PREC_MULDIV,     /* * / */
    PREC_UNARY,      /* # ! */
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
typedef struct ExprNode *(*InfixFn)(ExprNode *left, TokenStruct token,
                                    OpPrecedence precedence);

struct ExprNode
{
    enum {
        NumberExpr,
        StringExpr,
        NilExpr,
        BooleanExpr,
        VariableExpr,
        BinaryExpr,
        UnaryExpr,
        FunctionCallExpr
    } type;
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
    enum { AssignmentStat, IfElseStat, WhileLoopStat, FunctionCAllStat } type;
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
void traverse_stat_node_list(StatNodeList *chunk);

#endif // PARSER_H
