#include "env.h"
#include "ir.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

internal void emitError(const char *message)
{
    fprintf(stderr, "bytecode: %s\n", message);
    abort();
}

internal void appendOp(ByteCode *bytecode, OpCode op)
{
    if (bytecode->op_count >= bytecode->op_cap) {
        bytecode->op_cap = bytecode->op_cap == 0 ? 64u : bytecode->op_cap * 2u;
        OpCode *new_ops =
            realloc(bytecode->ops, bytecode->op_cap * sizeof(OpCode));
        if (new_ops == NULL) {
            emitError("out of memory");
        }
        bytecode->ops = new_ops;
    }
    bytecode->ops[bytecode->op_count++] = op;
}

internal void appendAssignment(ByteCode *bytecode, const char *name,
                               Register reg)
{
    if (bytecode->assign_count >= bytecode->assign_cap) {
        bytecode->assign_cap =
            bytecode->assign_cap == 0 ? 16u : bytecode->assign_cap * 2u;
        AssignResult *new_assigns = realloc(
            bytecode->assigns, bytecode->assign_cap * sizeof(AssignResult));
        if (new_assigns == NULL) {
            emitError("out of memory");
        }
        bytecode->assigns = new_assigns;
    }
    bytecode->assigns[bytecode->assign_count].name = name;
    bytecode->assigns[bytecode->assign_count].reg = reg;
    bytecode->assign_count++;
}

typedef struct
{
    const char *name;
    Register reg;
} SymbolEntry;

typedef struct
{
    SymbolEntry *entries;
    u32 count;
    u32 cap;
} SymbolTable;

internal SymbolTable initSymbolTable(void)
{
    SymbolTable symbols = {NULL, 0, 0};
    return symbols;
}

internal void destroySymbolTable(SymbolTable *symbols)
{
    free(symbols->entries);
    symbols->entries = NULL;
    symbols->count = 0;
    symbols->cap = 0;
}

internal void symbolGrow(SymbolTable *symbols)
{
    if (symbols->count < symbols->cap) {
        return;
    }

    symbols->cap = symbols->cap == 0 ? 8u : symbols->cap * 2u;
    SymbolEntry *new_entries =
        realloc(symbols->entries, symbols->cap * sizeof(SymbolEntry));
    if (new_entries == NULL) {
        emitError("out of memory");
    }
    symbols->entries = new_entries;
}

internal void symbolSet(SymbolTable *symbols, const char *name, Register reg)
{
    for (u32 i = 0; i < symbols->count; ++i) {
        if (strcmp(symbols->entries[i].name, name) == 0) {
            symbols->entries[i].reg = reg;
            return;
        }
    }

    symbolGrow(symbols);
    symbols->entries[symbols->count].name = name;
    symbols->entries[symbols->count].reg = reg;
    symbols->count++;
}

internal bool symbolGet(SymbolTable *symbols, const char *name, Register *reg)
{
    for (u32 i = 0; i < symbols->count; ++i) {
        if (strcmp(symbols->entries[i].name, name) == 0) {
            *reg = symbols->entries[i].reg;
            return true;
        }
    }
    return false;
}

internal Register emitLiteral(ByteCode *bytecode, Literal value)
{
    Register dest = bytecode->reg++;
    appendOp(bytecode,
             (OpCode){.type = LITERAL, .dest = dest, .code.load.value = value});
    return dest;
}

internal Register emitArith(ByteCode *bytecode, ArithOp op, Register a,
                            Register b)
{
    Register dest = bytecode->reg++;
    appendOp(bytecode, (OpCode){.type = REGISTER,
                                .dest = dest,
                                .code.arith = {.op = op, .a = a, .b = b}});
    return dest;
}

internal Register emitMov(ByteCode *bytecode, Register src)
{
    return emitArith(bytecode, CODE_MOV, src, src);
}

internal ArithOp arithOpFromToken(u32 op)
{
    switch (op) {
    case TK_ADD: {
        return CODE_ADD;
    } break;
    case TK_SUB: {
        return CODE_SUB;
    } break;
    case TK_MUL: {
        return CODE_MUL;
    } break;
    case TK_DIV: {
        return CODE_DIV;
    } break;
    default: {
        return CODE_UNKNOWN;
    } break;
    }
}

internal Register emitExpression(Parser *parser, Expr *expr, ByteCode *bytecode,
                                 Env *env, SymbolTable *symbols)
{
    switch (expr->type) {
    case ExprNumber: {
        return emitLiteral(bytecode, expr->as.number);
    } break;
    case ExprVariable: {
        Register src;
        if (symbolGet(symbols, expr->as.variable, &src)) {
            return emitMov(bytecode, src);
        }
        return emitLiteral(bytecode, envGet(env, expr->as.variable));
    } break;
    case ExprGroup: {
        return emitExpression(parser, getExpression(parser, expr->as.group),
                              bytecode, env, symbols);
    } break;
    case ExprUnary: {
        if (expr->as.unary.op != TK_SUB) {
            return emitLiteral(bytecode, 0.0);
        }
        Register operand =
            emitExpression(parser, getExpression(parser, expr->as.unary.l_expr),
                           bytecode, env, symbols);
        Register zero = emitLiteral(bytecode, 0.0);
        return emitArith(bytecode, CODE_SUB, zero, operand);
    } break;
    case ExprBinary: {
        ArithOp op = arithOpFromToken(expr->as.binary.op);
        if (op == CODE_UNKNOWN) {
            return emitLiteral(bytecode, 0.0);
        }

        AST_INDEX left_idx = expr->as.binary.l_expr;
        AST_INDEX right_idx = expr->as.binary.r_expr;
        if (left_idx == AST_NONE || right_idx == AST_NONE) {
            emitError("missing binary child");
        }

        Register a = emitExpression(parser, getExpression(parser, left_idx),
                                    bytecode, env, symbols);
        Register b = emitExpression(parser, getExpression(parser, right_idx),
                                    bytecode, env, symbols);
        return emitArith(bytecode, op, a, b);
    } break;
    default: {
        return emitLiteral(bytecode, 0.0);
    } break;
    }
}

internal void emitStatement(Parser *parser, ByteCode *bytecode, Env *env,
                            SymbolTable *symbols, Statement *stmt)
{
    switch (stmt->type) {
    case StmtBlock: {
        AST_INDEX block_idx = stmt->as.block.block_idx;
        while (block_idx != AST_NONE) {
            Statement *child = getStatement(parser, block_idx);
            emitStatement(parser, bytecode, env, symbols, child);
            block_idx = child->next;
        }
    } break;
    case StmtAssignment: {
        Expr *value = getExpression(parser, stmt->as.assignment.value);
        Register result = emitExpression(parser, value, bytecode, env, symbols);
        symbolSet(symbols, stmt->as.assignment.name, result);
        appendAssignment(bytecode, stmt->as.assignment.name, result);
    } break;
    case StmtLocalVar: {
        Register result;
        if (stmt->as.local_var.value == AST_NONE) {
            result = emitLiteral(bytecode, 0.0);
        } else {
            Expr *value = getExpression(parser, stmt->as.local_var.value);
            result = emitExpression(parser, value, bytecode, env, symbols);
        }
        symbolSet(symbols, stmt->as.local_var.name, result);
        appendAssignment(bytecode, stmt->as.local_var.name, result);
    } break;
    case StmtExpr: {
        Expr *value = getExpression(parser, stmt->as.expr_stmt.expr);
        emitExpression(parser, value, bytecode, env, symbols);
    } break;
    default: {
        return;
    } break;
    }
}

ByteCode *emitByteCode(Parser *parser, Statement *root, Env *env)
{
    ByteCode *bytecode = calloc(1, sizeof(ByteCode));
    if (bytecode == NULL) {
        emitError("out of memory");
    }

    SymbolTable symbols = initSymbolTable();
    emitStatement(parser, bytecode, env, &symbols, root);
    destroySymbolTable(&symbols);
    return bytecode;
}

void destroyByteCode(ByteCode *bytecode)
{
    if (bytecode == NULL) {
        return;
    }
    free(bytecode->ops);
    free(bytecode->assigns);
    free(bytecode);
}
