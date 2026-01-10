#include "symboltable.h"
#include "map.h"

#include <stdlib.h>

/**
 * Contain the Abstraction over map.h to and provides some features of symbol
 * table
 **/

struct SymTable
{
    hmap *symmap;
    struct SymTable *outer;
};

SymTable *G_SymTable = NULL;

SymTable *STable_create(SymTable *outer)
{
    SymTable *STable = malloc(sizeof(SymTable));
    if (STable == NULL) {
        return NULL;
    }
    hmap *symmap = map_create();
    if (symmap == NULL) {
        free(STable);
        return NULL;
    }
    STable->symmap = symmap;
    STable->outer = outer;
    return STable;
}

void STable_destroy(SymTable *STable)
{
    map_destroy(STable->symmap);
    free(STable);
}

const char *STable_set_entry(SymTable *STable, const char *key, Symbol *Sym)
{
    return map_set_entry(STable->symmap, key, (void *)Sym);
}

Symbol *STable_get_value(SymTable *STable, const char *key)
{
    if (STable == NULL) {
        return NULL;
    }

    Symbol *sym = (Symbol *)map_get_value(STable->symmap, key);

    if (sym == NULL && STable->outer != NULL) {
        return STable_get_value(STable->outer, key);
    }

    return sym;
}

void G_STable_create() { G_SymTable = STable_create(NULL); }

/* Symbol Creations */

struct Symbol
{
    SymType type;
    bool is_constant;
    int scope;
};

Symbol *Sym_create(SymType type, bool is_constant, int scope)
{
    Symbol *Sym = malloc(sizeof(Symbol));
    if (Sym == NULL) {
        return NULL;
    }

    Sym->type = type;
    Sym->is_constant = is_constant;
    Sym->scope = scope;

    return Sym;
}
