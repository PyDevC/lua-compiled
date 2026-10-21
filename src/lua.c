#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "cmdline.h"
#include "parser.h"

#include <stdio.h>

void walk_ast(Parser *parser, Statement *root)
{
    ASTIndex stmt_idx = root->as.block.first_stmt;
    while (stmt_idx != ASTNone) {
        Statement *stmt = get_statement(parser, stmt_idx);
        if (stmt != NULL && stmt->type == StmtAssignment) {
            printf("%s = ", stmt->as.assignment.name);
            Expr *value = get_expression(parser, stmt->as.assignment.value);
            if (value != NULL) {
                printf("<value>");
            }
        } else {
            printf("statement is null\n");
            break;
        }
        printf("\n");
        stmt_idx = stmt->next;
    }
}

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
        walk_ast(parser, root_chunk);
        return 0;
    }
}
