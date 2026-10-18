#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "cmdline.h"
#include "parser.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc == 1) {
        print_cmdline_usage();
        return 1;
    } else if (argc == 2) {
        setup_cmdline_args(argc, argv);
        LuaCmdlineParam params = get_params();
        Parser *parser = init_parser(params.filename);
        ASTIndex root_chunk_idx = parse_block(parser);
        Statement *root_chunk = get_statement(parser, root_chunk_idx);
        printf("%d\n", root_chunk->type);
        return 0;
    }
}
