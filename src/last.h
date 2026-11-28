#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "defs.h"

#define AST_INDEX u32
#define AST_NONE ((u32) - 1)

typedef enum ExprType {
    ExprNil,
    ExprBoolean,
    ExprNumber,
    ExprString,
    ExprVariable,
    ExprUnary,
    ExprBinary,
    ExprGroup,
    ExprFuncCall,
} ExprType;

typedef struct Expr
{
    ExprType type;
    AST_INDEX next;
    union
    {
        bool boolean;
        double number;
        const char *string;
        const char *variable;
        AST_INDEX group;

        struct
        {
            AST_INDEX callee;
            AST_INDEX argv; // First Argument
            u32 argc;
        } call;

        struct
        {
            u32 op;
            AST_INDEX l_expr;
        } unary;

        struct
        {
            u32 op;
            AST_INDEX l_expr;
            AST_INDEX r_expr;
        } binary;

    } as;
} Expr;

typedef enum {
    StmtBlock,
    StmtLocalVar,
    StmtAssignment,
    StmtIfElse,
    StmtWhile,
    StmtRepeatUntil,
    StmtForLoop,
    StmtFuncCall,
    StmtFunction,
    StmtExpr,
} StmtType;

typedef struct
{
    StmtType type;
    AST_INDEX next;

    union
    {
        struct
        {
            AST_INDEX block_idx; // Index of first Statement in the Block
        } block;

        struct
        {
            const char *name;
            AST_INDEX value;
        } local_var;

        struct
        {
            const char *name;
            AST_INDEX value;
        } assignment;

        struct
        {
            AST_INDEX expr;
        } expr_stmt;

        struct
        {
            // Can be if, else or elif
            AST_INDEX condition_expr;
            AST_INDEX block_idx; // Index of first Statement in the Block
            AST_INDEX next_elif_or_else;
        } if_else_stmt;

        struct
        {
            AST_INDEX condition_expr;
            AST_INDEX block_idx; // Index of first Statement in the Block
            bool is_repat_until;
        } while_or_repeat_stmt;

        struct
        {
            const char *var;
            AST_INDEX start_expr;
            AST_INDEX end_expr;
            AST_INDEX step_expr;
            AST_INDEX block_idx; // Index of first Statement in the Block
        } for_stmt;

        struct
        {
            AST_INDEX callee;
        } funccall;

        struct
        {
            const char *name;
            AST_INDEX argv; // First Argument
            u32 argc;
            AST_INDEX block_idx; // Index of first Statement in the Block
        } func_stmt;
    } as;
} Statement;
