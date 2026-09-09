#ifndef CODEGEN_H
#define CODEGEN_H

#include "ir.h"
#include "parser.h"
#include "symboltable.h"

typedef struct
{
    IRInstr *head;
    IRInstr *tail;
} IRStream;

IROperand generate_expr_ir(ExprNode *expr, SymTable *STable, IRStream *stream);
void emit(IRStream *stream, IROp op, IROperand dst, IROperand src1,
          IROperand src2);
void generate_assembly(IRStream *stream);
IRStream *IRStream_create();

#endif // CODEGEN_H
