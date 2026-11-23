/* ========================================================================

   $File: semantic.h
   $Date: 2026/11/20
   $Revision: 1

   Semantic analysis and strict type inference. The pass resolves every name
   to a flat symbol slot, infers one type per variable, checks call arity and
   statement placement, and annotates AST nodes so the IR builder no longer
   needs to know anything about scopes or source-level names.

   ======================================================================== */

#ifndef LUA_SEMANTIC_H
#define LUA_SEMANTIC_H

#include "parser.h"
#include "types.h"

typedef enum builtin_kind
{
    Builtin_None,
    Builtin_Print,
    Builtin_Type,
} builtin_kind;

typedef struct function_signature
{
    string Name;
    char *Label;
    u32 ParamCount;
    lua_type *Params;
    lua_type Return;
    u32 FirstParamSymbol;
    b32 IsBuiltin;
    builtin_kind Builtin;
} function_signature;

typedef struct semantic_symbol
{
    string Name;
    lua_type Type;
    u32 Slot;
    u32 OwnerFunction;
    b32 IsGlobal;
} semantic_symbol;

typedef struct semantic_context
{
    Parser *Parser;
    memory_arena *Arena;
    message_log *Log;
    u32 StartErrorCount;

    semantic_symbol *Symbols;
    u32 SymbolCount;
    u32 SymbolCapacity;

    u32 *ScopeMarks;
    u32 *ScopeEnds;
    u32 ScopeCount;
    u32 ScopeCapacity;

    u32 *FunctionLocalCounts;
    u32 FunctionLocalCapacity;

    function_signature *Functions;
    u32 FunctionCount;
    u32 FunctionCapacity;

    u32 GlobalCount;

    u32 CurrentFunction;
    u32 LoopDepth;
} semantic_context;

#define ChunkFunctionIndex 2
#define SymbolNone ((u32)-1)

void InitSemantic(semantic_context *Context, Parser *Parser,
                  memory_arena *Arena, message_log *Log);
void AnalyzeSemantic(semantic_context *Context, ASTIndex FirstStatement);

internal semantic_symbol *GetSymbol(semantic_context *Context, u32 Index);
internal function_signature *GetFunction(semantic_context *Context, u32 Index);

#endif /* LUA_SEMANTIC_H */
