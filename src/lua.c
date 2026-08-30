#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "errors.h"
#include "lexer.h"
#include "parser.h"
#include "resolve.h"
#include "symboltable.h"
#include "cmdline.h"

int main(int argc, char **argv)
{
    if (argc == 1) {
        print_cmdline_usage();
        return 1;
    } else if (argc == 2) {
        setup_cmdline_args(argc, argv);
        char *filename = argv[1];
        init_lexer(filename);
        G_STable_create(); /* Initialize the Global Symbol Table */
        StatNodeList *chunk = parse_chunk();
        return 0;
        D(fprintf(stdout, "DEBUG: Starting Type Resolution\n"));
        SymTable *global_table = STable_create(NULL);
        IRStream *ir_stream = IRStream_create();
/* ADDED Just to debug will be removed in future */
#ifdef DEBUG_LUA
        traverse_stat_node_list(chunk);
#endif

        resolve_node_stat_list(chunk, G_SymTable);
        StatNodeList *curr = chunk;
        while (curr != NULL) {
            if (curr->stat->type == AssignmentStat) {
                generate_expr_ir(curr->stat->data.assingment_stat.expr,
                                 global_table, ir_stream);
            }
            curr = curr->next;
        }

        // 4. Final Output
        generate_assembly(ir_stream);

        return 0;
    }
}
