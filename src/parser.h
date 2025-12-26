#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef enum {
    PREC_NULL,
    PREC_NIL,
    PREC_OR,
    PREC_AND,
    PREC_ADDSUB,
    PREC_MULDIV,
    PREC_POW,
    PREC_UNARY
} OpPrecedence;

/* Forward Declarations */
typedef struct StatNodeList StatNodeList; /* Chunk or Block */
typedef struct StatNode StatNode;
typedef struct ExprNode ExprNode;
typedef struct NameList NameList;
typedef struct VarNode VarNode;
typedef struct ExprNode *(*PrefixFn)(TokenStruct token,
                                     OpPrecedence precedence);
typedef struct ExprNode *(*InfixFn)(ExprNode *left, TokenType type,
                                    OpPrecedence precedence);

struct ExprNode
{
    enum {
        NilExpr,
        BooleanExpr,
        NumberExpr,
        StringExpr,
        PrefixExpr,
        BinaryExpr,
        UnaryExpr
    } type;

    union
    {
        double number_expr;
        char *string_expr;
        VarNode *prefix_expr_var;

        struct
        {
            TokenType op;
            ExprNode *left;
            ExprNode *right;
        } binary_expr;

        struct
        {
            char *op;
            ExprNode *right;
        } unary_expr;

    } data;
};

struct VarNode
{
    enum { NameVar } type;
    char *name;
};

struct StatNode
{
    enum {
        AssignmentStat,
        LocalVarListDefStat,
        FunctionCallStat,
    } type;

    union
    {

        struct
        {
            VarNode *var;
            ExprNode *expr;
        } assingment_stat;

        struct
        {
            NameList *namelist;
            ExprNode *expr;
        } localvarlist_stat;

    } data;
};

struct StatNodeList
{
    StatNode *stat;
    StatNodeList *next;
};

struct NameList
{
    char *name;
    NameList *next;
};

typedef struct
{
    PrefixFn prefix;
    InfixFn infix;
    OpPrecedence precedence;
} ParseRule;

StatNodeList *parse_chunk(); /* This is the main node that should be exposed */

#endif // PARSER_H
