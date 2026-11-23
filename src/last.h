/* ========================================================================

   $File: last.h
   $Date: 2026/11/20
   $Revision: 1

   "last.h" is the old name for the tree (Lua AST). The layout is unchanged
   in spirit: dense arrays of nodes, ASTIndex instead of pointers, and a
   "next" link for sibling lists. Two node kinds were added (return/break)
   because function.lua actually uses `return`.

   ======================================================================== */

#ifndef LUA_LAST_H
#define LUA_LAST_H

#include "defs.h"
#include "lexer.h"
#include "types.h"

#define ASTIndex u32
#define ASTNone ((u32)-1)

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
    ASTIndex next;

    /* Byte offset of the token that started this node. Line/column are
     * computed only when a diagnostic is printed. */
    u32 Offset;

    /* Filled by the semantic pass. SymbolIndex is ASTNone for expressions
     * that do not name a variable. */
    u32 SymbolIndex;
    lua_type Type;

    /* Set while the semantic pass visits the callee of a direct call. Builtins
     * lower inline and have no first-class address, so using `print` as a value
     * is rejected everywhere except this position. */
    b32 IsCallCallee;

    union
    {
        b32 boolean;
        f64 num;
        const char *string;
        const char *variable;
        ASTIndex group;

        struct
        {
            ReservedToken op;
            ASTIndex l_expr;
        } unary;

        struct
        {
            ReservedToken op;
            ASTIndex l_expr;
            ASTIndex r_expr;
        } binary;

        struct
        {
            ASTIndex callee;
            ASTIndex i_arg;
            u32 argc;
        } call;
    } as;
} Expr;

typedef enum StmtType {
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
    StmtReturn,
    StmtBreak,
} StmtType;

typedef struct Statement
{
    StmtType type;
    ASTIndex next;

    /* Byte offset of the token that started this node. */
    u32 Offset;

    /* Filled by the semantic pass; Type.SignatureIndex identifies a function
     * definition when type is StmtFunction. */
    u32 SymbolIndex;
    lua_type Type;

    union
    {
        struct
        {
            ASTIndex block_idx; /* first statement in the block */
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
        } assign;

        ASTIndex expr_stmt;

        struct
        {
            ASTIndex cond;
            ASTIndex block_idx;
            ASTIndex next_el; /* next elseif or else statement */
        } if_stmt;

        struct
        {
            ASTIndex cond;
            ASTIndex block_idx;
            b32 is_repeat_until;
        } while_or_repeat_stmt;

        struct
        {
            const char *var;
            ASTIndex start;
            ASTIndex end_;
            ASTIndex step;
            ASTIndex block_idx;
        } for_stmt;

        ASTIndex funccall;

        struct
        {
            const char *name;
            ASTIndex first_arg;
            u32 argc;
            ASTIndex first_stmt;
        } func_stmt;

        ASTIndex return_value;
    } as;
} Statement;

#endif /* LUA_LAST_H */
