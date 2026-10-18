#pragma once

#include "map.h"

typedef struct SymTable SymTable;
typedef struct Symbol Symbol;

typedef enum {
    S_NIL,
    S_INT,
    S_FLOAT,
    S_STRING,
    S_BOOLEAN,
    S_FUNCTION,
    S_VARIABLE,
} SymType;

typedef enum {
    VAR_NODE,
    FUNCTION_CALL,
} SymCategory;

struct SymTable
{
    hmap *symmap;
    struct SymTable *outer;
};

struct Symbol
{
    SymType type;
    SymCategory category;
    void *symbol;
};

SymTable *STable_create(SymTable *outer);
void STable_destroy(SymTable *table);
const char *STable_set_entry(SymTable *STable, const char *key, Symbol *Sym);
Symbol *STable_get_value(SymTable *STable, const char *key);
Symbol *Sym_create(SymType type, SymCategory category, void *symbol);

/* Global Symbol Table */

extern SymTable *G_SymTable;

void G_STable_create();
