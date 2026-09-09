#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "cmdline.h"
#include "codegen.h"
#include "errors.h"
#include "resolve.h"

int main(int argc, char **argv)
{
    if (argc == 1) {
        print_cmdline_usage();
        return 1;
    } else if (argc == 2) {
        // Setup
        setup_cmdline_args(argc, argv);
        Param_T params = get_params();
        init_parser(params.filename);

        // parsing
        StatNodeList *lua_ast = parse_chunk();
        D(fprintf(stdout, "DEBUG: Starting Type Resolution\n"));
        IRStream *ir_stream = IRStream_create();

/* ADDED Just to debug will be removed in future */
#ifdef DEBUG_LUA
        traverse_stat_node_list(chunk);
#endif

        resolve_node_stat_list(lua_ast, G_SymTable);
        StatNodeList *curr = lua_ast;
        while (curr != NULL) {
            if (curr->stat->type == AssignmentStat) {
                generate_expr_ir(curr->stat->data.assingment_stat.expr,
                                 G_SymTable, ir_stream);
            }
            curr = curr->next;
        }

        generate_assembly(ir_stream);

        return 0;
    }
}
