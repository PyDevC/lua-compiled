#ifndef RESOLVE_H
#define RESOLVE_H

#include "parser.h"
#include "symboltable.h"

void resolve_node_stat_list(StatNodeList *block, SymTable *STable);
void resolve_node_stat(StatNode *statement, SymTable *STable);
void resolve_node_var(VarNode *variable, SymTable *STable);
void resolve_node_expr(ExprNode *expression, SymTable *STable, const char *key);

#endif // RESOLVE_H
