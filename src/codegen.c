#include "codegen.h"
#include "errors.h"
#include <stdlib.h>
#include <string.h>

static int temp_vaiable_count = 0;

char *new_temp()
{
    char *buf = malloc(10);
    sprintf(buf, "t%d", temp_vaiable_count++);
    return buf;
}

IRStream *IRStream_create()
{
    IRStream *stream = malloc(sizeof(IRStream));
    if (stream == NULL) {
        E(fprintf(stderr, "Error: Memory allocation failed for IRStream\n"));
        return NULL;
    }

    stream->head = NULL;
    stream->tail = NULL;

    return stream;
}

void emit(IRStream *stream, IROp op, IROperand dst, IROperand src1,
          IROperand src2)
{
    IRInstr *instr = malloc(sizeof(IRInstr));
    instr->op = op;
    instr->dst = dst;
    instr->src1 = src1;
    instr->src2 = src2;
    instr->next = NULL;

    if (stream->tail) {
        stream->tail->next = instr;
        stream->tail = instr;
    } else {
        stream->head = stream->tail = instr;
    }
}

IROperand generate_expr_ir(ExprNode *expr, SymTable *STable, IRStream *stream)
{
    switch (expr->type) {
    case NumberExpr: {
        IROperand res = {.type = OP_CONST,
                         .data.val = expr->data.literalnumber};
        return res;
    }
    case VariableExpr: {
        IROperand res = {.type = OP_VAR,
                         .data.name = strdup(expr->data.var->name)};
        return res;
    }
    case BinaryExpr: {
        IROperand left =
            generate_expr_ir(expr->data.binary_expr.left, STable, stream);
        IROperand right =
            generate_expr_ir(expr->data.binary_expr.right, STable, stream);

        IROperand dest = {.type = OP_TEMP, .data.name = new_temp()};

        IROp op;
        if (expr->data.binary_expr.op.type == ADD)
            op = IR_ADD;

        emit(stream, op, dest, left, right);
        return dest;
    }
    default:
        E(fprintf(stderr, "Invalid Operation\n"));
        break;
    }
}

void generate_assembly(IRStream *stream)
{
    IRInstr *curr = stream->head;
    while (curr) {
        if (curr->op == IR_ADD) {
            printf("  mov rax, %s\n", curr->src1.data.name);
            printf("  add rax, %s\n", curr->src2.data.name);
            printf("  mov %s, rax\n", curr->dst.data.name);
        }
        curr = curr->next;
    }
}
