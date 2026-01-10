#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef struct SymTable SymTable;
typedef struct Symbol Symbol;

typedef enum {
    S_INT,
    S_FLOAT,
    S_STRING,
    S_FUNCTION,
    S_NIL,
} SymType;

SymTable *STable_create(SymTable *outer);
void STable_destroy(SymTable *table);
const char *STable_set_entry(SymTable *STable, const char *key, Symbol *Sym);
Symbol *STable_get_value(SymTable *STable, const char *key);

/* Global Symbol Table */

extern SymTable *G_SymTable;

void G_STable_create();

#endif // SYMBOL_TABLE_H
