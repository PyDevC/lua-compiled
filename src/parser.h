#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

/* Forward Declarations */
typedef struct StatNodeList StatNodeList; /* Chunk or Block */
typedef struct StatNode StatNode;
typedef struct ExprNode ExprNode;
typedef struct NameList NameList;
typedef struct VarNode VarNode;

struct ExprNode {
    enum {
        NilExpr,
        FalseExpr,
        TrueExpr,
        NumberExpr,
        StringExpr,
        PrefixExpr,
        BinaryExpr,
        UnaryExpr
    } type;

    union {
        double number_expr;
        char *string_expr;
        VarNode *prefix_expr_var;

        struct {
            char *op;
            ExprNode *left;
            ExprNode *right;
        } binary_expr;

        struct {
            char *op;
            ExprNode *right;
        } unary_expr;

    } data;
};

struct VarNode {
    enum { NameVar } type;
    char *name;
};

struct StatNode {
    enum {
        AssignmentStat,
        LocalVarListDefStat,
    } type;

    union {

        struct {
            VarNode *var;
            ExprNode *expr;
        } assingment_stat;

        struct {
            NameList *namelist;
            ExprNode *expr;
        } localvarlist_stat;

    } data;
};

struct StatNodeList {
    StatNode *stat;
    StatNodeList *next;
};

struct NameList {
    char *name;
    NameList *next;
};

StatNodeList *parse_chunk(); /* This is the main node that should be exposed */

#endif // PARSER_H
