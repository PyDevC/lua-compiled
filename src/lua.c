#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "errors.h"
#include "lexer.h"
#include "parser.h"
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
        init_trace_stack(&global_tracestack);
        init_lexer(filename);
        StatNodeList *chunk = parse_chunk();
        printf("%d ", chunk->stat->type);
        return 0;
    }
}
