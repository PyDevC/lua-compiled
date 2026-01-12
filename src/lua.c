#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "errors.h"
#include "lexer.h"
#include "parser.h"
#include "resolve.h"
#include "symboltable.h"
#include <stdio.h>

void usage()
{
    /* Display usage of the compiler with the options available */
    printf("lua [flags] [filename]\n");
    printf("ex: lua hello.lua\n");
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        usage();
        return 1;
    } else if (argc == 2) {
        char *filename = argv[1];
        init_lexer(filename);
        G_STable_create(); /* Initialize the Global Symbol Table */
        StatNodeList *chunk = parse_chunk();
        D(fprintf(stdout, "DEBUG: Starting Type Resolution\n"));
/* ADDED Just to debug will be removed in future */
#ifdef DEBUG_LUA
        traverse_stat_node_list(chunk);
#endif

        resolve_node_stat_list(chunk, G_SymTable);
        return 0;
    }
}
