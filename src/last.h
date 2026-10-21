#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define ASTIndex uint32_t
#define u32 uint32_t
#define ASTNone ((uint32_t)-1)

typedef enum ExprType {
    ExprNil,
    ExprBoolean,
    ExprNumber,
    ExprString,
    ExprVariable,
    ExprCall,
    ExprUnary,
    ExprBinary,
    ExprGroup,
} ExprType;

typedef struct Expr
{
    ExprType type;
    union
    {
        bool boolean;
        double number;
        const char *string;
        const char *variable;
        ASTIndex group;

        struct
        {
            ASTIndex callee;
            ASTIndex first_arg;
            u32 arg_count;
        } call;

        struct
        {
            u32 op;
            ASTIndex l_expr;
        } unary;

        struct
        {
            u32 op;
            ASTIndex l_expr;
            ASTIndex r_expr;
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
    StmtExpr,
} StmtType;

typedef struct
{
    StmtType type;
    ASTIndex next;

    union
    {
        struct
        {
            ASTIndex first_stmt;
        } block;

        struct
        {
            const char *name;
            ASTIndex value;
        } local_var;

        struct
        {
            const char *name;
            ASTIndex value;
        } assignment;

        struct
        {
            ASTIndex expr;
        } expr_stmt;

        struct
        {
            // Can be if, else or elif
            ASTIndex condition_expr;
            ASTIndex first_stmt;
            ASTIndex next_elif_or_else;
        } if_else_stmt;

        struct
        {
            ASTIndex condition_expr;
            ASTIndex first_stmt;
            bool is_repat_until;
        } while_or_repeat_stmt;

        struct
        {
            const char *var;
            ASTIndex start_expr;
            ASTIndex end_expr;
            ASTIndex step_expr;
            ASTIndex first_stmt;
        } for_stmt;
    } as;
} Statement;
