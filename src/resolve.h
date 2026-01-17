#ifndef RESOLVE_H
#define RESOLVE_H

#include "ir.h"
#include "parser.h"
#include "symboltable.h"

typedef struct
{
    IRInstr *head;
    IRInstr *tail;
} IRStream;

void resolve_node_stat_list(StatNodeList *block, SymTable *STable);
void resolve_node_stat(StatNode *statement, SymTable *STable);
void resolve_node_var(VarNode *variable, SymTable *STable);
void resolve_node_expr(ExprNode *expression, SymTable *STable, const char *key);
void emit(IRStream *stream, IROp op, IROperand dst, IROperand src1,
          IROperand src2);
IROperand generate_expr_ir(ExprNode *expr, SymTable *STable, IRStream *stream);
void generate_assembly(IRStream *stream);
IRStream *IRStream_create();

#endif // RESOLVE_H
