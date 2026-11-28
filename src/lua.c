#define CLUA_VERSION 0.0.1
#define CODENAME LUA_PR

#include "cmdline.h"
#include "env.h"
#include "ir.h"
#include "vm.h"

#include <inttypes.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc == 1) {
        printCmdlineUsage();
        return 1;
    }

    if (argc != 2) {
        printCmdlineUsage();
        return 1;
    }

    setupCmdlineArgs(argc, argv);
    LuaCmdlineParam params = getParams();
    Parser *parser = initParser(params.filename);
    AST_INDEX root_chunk_idx = parseBlock(parser);
    Statement *root_chunk = getStatement(parser, root_chunk_idx);

#ifdef PARSER_DEBUG
    printStatement(parser, root_chunk);
#endif

#ifdef DEBUG_LUA
    printf("root block: %" PRIu32 "\n", root_chunk_idx);
#endif

    Env *env = initEnv();
    ByteCode *bytecode = emitByteCode(parser, root_chunk, env);
    VM *vm = initVM(bytecode->reg);
    runByteCode(vm, bytecode);
    applyAssignments(env, bytecode, vm);

    if (envWasSet(env, "ret")) {
        printf("$ ret = %g\n", envGet(env, "ret"));
    }

    destroyVM(vm);
    destroyByteCode(bytecode);
    destroyEnv(env);
    destroyParser(parser);
    return 0;
}
