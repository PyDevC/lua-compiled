#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "cmdline.h"
#include "codegen.h"
#include "parser.h"
#include "resolve.h"

int main(int argc, char **argv)
{
    if (argc == 1) {
        print_cmdline_usage();
        return 1;
    } else if (argc == 2) {
        // Setup
        setup_cmdline_args(argc, argv);
        LuaCmdlineParam params = get_params();
        LexerState *lex_state = init_parser(params.filename);

        // parsing
        StatNodeList *lua_ast = parse_chunk(lex_state);

/* ADDED Just to debug will be removed in future */
#ifdef DEBUG_LUA
        traverse_stat_node_list(lua_ast);
#endif

        // semantics check
        resolve_node_stat_list(lua_ast, G_SymTable);
        // TODO: emitIR as ir stream
        // generate_assembly(ir_stream);

        return 0;
    }
}
