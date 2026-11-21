/* ========================================================================

   $File: semantic.c
   $Date: 2026/11/20
   $Revision: 1

   One pass over the tree does three things at once: resolve names to slots,
   infer/check strict types, and annotate AST nodes. Function signatures are
   registered in a small pre-pass so recursion and forward calls work.

   ======================================================================== */

#include "semantic.h"

internal void AnalyzeList(semantic_context *Context, ASTIndex First);
internal void AnalyzeStatement(semantic_context *Context, ASTIndex Index);
internal void AnalyzeExpr(semantic_context *Context, ASTIndex Index);

internal void
SemanticError(semantic_context *Context, char *Format, ...)
{
    Context->HadError = 1;
    fprintf(stderr, "semantic error: ");

    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);
    fprintf(stderr, "\n");
}

internal void
PushScope(semantic_context *Context)
{
    Assert(Context->ScopeCount < Context->ScopeCapacity);
    u32 Index = Context->ScopeCount++;
    Context->ScopeMarks[Index] = Context->SymbolCount;
    Context->ScopeEnds[Index] = Context->SymbolCount;
}

internal void
PopScope(semantic_context *Context)
{
    Assert(Context->ScopeCount > 0);
    u32 Index = --Context->ScopeCount;
    /* Symbols are never reused: the IR keeps their indices. Closing a scope
     * only records where it ended for name lookup. */
    Context->ScopeEnds[Index] = Context->SymbolCount;
}

internal semantic_symbol *
GetSymbol(semantic_context *Context, u32 Index)
{
    Assert(Index != SymbolNone);
    Assert(Index < Context->SymbolCount);
    return(Context->Symbols + Index);
}

internal function_signature *
GetFunction(semantic_context *Context, u32 Index)
{
    Assert(Index < Context->FunctionCount);
    return(Context->Functions + Index);
}

internal semantic_symbol *
FindSymbol(semantic_context *Context, string Name)
{
    semantic_symbol *Result = 0;

    for(u32 ScopeIndex = Context->ScopeCount; ScopeIndex > 0; --ScopeIndex)
    {
        u32 Mark = Context->ScopeMarks[ScopeIndex - 1];
        u32 End = Context->ScopeEnds[ScopeIndex - 1];
        for(u32 Index = End; Index > Mark; --Index)
        {
            semantic_symbol *Symbol = Context->Symbols + (Index - 1);
            if(StringsEqual(Symbol->Name, Name))
            {
                Result = Symbol;
                break;
            }
        }
        if(Result)
        {
            break;
        }
    }

    if(!Result)
    {
        /* Globals are appended as they are discovered, so they are not
         * necessarily the first GlobalCount entries when locals came first. */
        for(u32 Index = 0; Index < Context->SymbolCount; ++Index)
        {
            semantic_symbol *Symbol = Context->Symbols + Index;
            if(Symbol->IsGlobal && StringsEqual(Symbol->Name, Name))
            {
                Result = Symbol;
                break;
            }
        }
    }

    return(Result);
}

internal u32
FindBuiltinSignature(semantic_context *Context, string Name)
{
    u32 Result = SymbolNone;
    for(u32 Index = 0; Index < 2; ++Index)
    {
        if(StringsEqual(Context->Functions[Index].Name, Name))
        {
            Result = Index;
            break;
        }
    }
    return(Result);
}

internal u32
DeclareSymbol(semantic_context *Context, string Name, lua_type Type,
              b32 IsGlobal)
{
    Assert(Context->SymbolCount < Context->SymbolCapacity);
    u32 Result = Context->SymbolCount++;

    semantic_symbol *Symbol = Context->Symbols + Result;
    ZeroStruct(*Symbol);
    Symbol->Name = Name;
    Symbol->Type = Type;

    if(IsGlobal)
    {
        Symbol->IsGlobal = 1;
        Symbol->Slot = Context->GlobalCount++;
    }
    else
    {
        u32 Owner = Context->CurrentFunction;
        Assert(Owner < Context->FunctionLocalCapacity);
        Symbol->OwnerFunction = Owner;
        Symbol->Slot = Context->FunctionLocalCounts[Owner]++;
        if(Context->ScopeCount > 0)
        {
            /* Locals are appended, never recycled: the current scope owns the
             * symbol we just made, even after nested scopes are popped. */
            Context->ScopeEnds[Context->ScopeCount - 1] = Context->SymbolCount;
        }
    }

    return(Result);
}

internal u32
DeclareLocal(semantic_context *Context, string Name, lua_type Type)
{
    semantic_symbol *Existing = FindSymbol(Context, Name);
    if(Existing && !Existing->IsGlobal &&
        Existing->OwnerFunction == Context->CurrentFunction &&
        Context->ScopeCount > 0 &&
        (u32)(Existing - Context->Symbols) >=
        Context->ScopeMarks[Context->ScopeCount - 1])
    {
        /* A repeated `local x` in the same scope replaces the earlier
         * binding; a local in an outer scope is shadowed instead. */
        Existing->Type = Type;
        return((u32)(Existing - Context->Symbols));
    }

    return(DeclareSymbol(Context, Name, Type, 0));
}

internal u32
DeclareGlobal(semantic_context *Context, string Name, lua_type Type)
{
    return(DeclareSymbol(Context, Name, Type, 1));
}

internal b32
UnifyExprType(semantic_context *Context, ASTIndex ExprIndex, lua_type Type)
{
    Parser *P = Context->Parser;
    Expr *E = GetExpr(P, ExprIndex);
    b32 Result = UnifyType(&E->Type, Type);

    if(E->SymbolIndex != SymbolNone)
    {
        semantic_symbol *Symbol = GetSymbol(Context, E->SymbolIndex);
        if(!UnifyType(&Symbol->Type, Type))
        {
            Result = 0;
        }
        E->Type = Symbol->Type;
    }
    else if((E->type == ExprFuncCall) &&
        TypeIsValid(GetExpr(P, E->as.call.callee)->Type) &&
        (GetExpr(P, E->as.call.callee)->Type.Kind == Type_Function))
    {
        function_signature *Sig =
            GetFunction(Context,
                        GetExpr(P, E->as.call.callee)->Type.SignatureIndex);
        if(!UnifyType(&Sig->Return, Type))
        {
            Result = 0;
        }
        E->Type = Sig->Return;
    }

    if(!Result)
    {
        SemanticError(Context, "type mismatch: expected %s, got %s",
                      TypeName(Type), TypeName(E->Type));
    }
    return(Result);
}

internal void
RequireExprType(semantic_context *Context, ASTIndex ExprIndex,
                lua_type Required, char *What)
{
    if(!UnifyExprType(Context, ExprIndex, Required))
    {
        SemanticError(Context, "%s requires %s", What, TypeName(Required));
    }
}

internal void
RegisterFunctionList(semantic_context *Context, ASTIndex First);

internal void
RegisterFunctionStatement(semantic_context *Context, Statement *S)
{
    string Name = StringFromC((char *)S->as.func_stmt.name);

    if(FindSymbol(Context, Name))
    {
        SemanticError(Context, "'%.*s' is already declared", (int)Name.Count,
                      Name.Data);
        return;
    }

    Assert(Context->FunctionCount < Context->FunctionCapacity);
    u32 SigIndex = Context->FunctionCount++;
    function_signature *Sig = Context->Functions + SigIndex;
    ZeroStruct(*Sig);

    char *Label = PushArray(Context->Arena, 32, char);
    snprintf(Label, 32, "func_%u", SigIndex);
    Sig->Name = Name;
    Sig->Label = Label;
    Sig->ParamCount = S->as.func_stmt.argc;
    Sig->Params = PushArray(Context->Arena,
                            Sig->ParamCount ? Sig->ParamCount : 1, lua_type);
    for(u32 ParamIndex = 0; ParamIndex < Sig->ParamCount; ++ParamIndex)
    {
        Sig->Params[ParamIndex] = SimpleType(Type_Unknown);
    }
    Sig->Return = SimpleType(Type_Unknown);
    Sig->FirstParamSymbol = SymbolNone;

    lua_type FunctionType = SimpleType(Type_Function);
    FunctionType.SignatureIndex = SigIndex;

    u32 SymbolIndex = DeclareGlobal(Context, Name, FunctionType);
    S->SymbolIndex = SymbolIndex;
    S->Type = FunctionType;

    RegisterFunctionList(Context, S->as.func_stmt.first_stmt);
}

internal void
RegisterFunctionList(semantic_context *Context, ASTIndex First)
{
    Parser *P = Context->Parser;
    for(ASTIndex Index = First; Index != ASTNone;)
    {
        Statement *S = GetStatement(P, Index);
        switch(S->type)
        {
            case StmtFunction:
                {
                    RegisterFunctionStatement(Context, S);
                } break;

            case StmtBlock:
                {
                    RegisterFunctionList(Context, S->as.block.block_idx);
                } break;

            case StmtIfElse:
                {
                    RegisterFunctionList(Context, S->as.if_stmt.block_idx);
                    RegisterFunctionList(Context, S->as.if_stmt.next_el);
                } break;

            case StmtWhile:
            case StmtRepeatUntil:
                {
                    RegisterFunctionList(
                        Context, S->as.while_or_repeat_stmt.block_idx);
                } break;

            case StmtForLoop:
                {
                    RegisterFunctionList(Context, S->as.for_stmt.block_idx);
                } break;

            case StmtLocalVar:
            case StmtAssignment:
            case StmtExpr:
            case StmtFuncCall:
            case StmtReturn:
            case StmtBreak:
            {
                /* No nested function definitions are possible here. */
            } break;

            InvalidDefaultCase;
        }
        Index = S->next;
    }
}

void
InitSemantic(semantic_context *Context, Parser *P, memory_arena *Arena)
{
    ZeroStruct(*Context);
    Context->Parser = P;
    Context->Arena = Arena;

    u32 NodeCount = P->StmtCap + 16;
    Context->SymbolCapacity = NodeCount * 2;
    Context->ScopeCapacity = NodeCount;
    Context->FunctionCapacity = NodeCount;
    Context->FunctionLocalCapacity = NodeCount;

    Context->Symbols = PushArray(Arena, Context->SymbolCapacity, semantic_symbol);
    Context->ScopeMarks = PushArray(Arena, Context->ScopeCapacity, u32);
    Context->ScopeEnds = PushArray(Arena, Context->ScopeCapacity, u32);
    Context->FunctionLocalCounts = PushArray(Arena, Context->FunctionLocalCapacity, u32);
    Context->Functions = PushArray(Arena, Context->FunctionCapacity, function_signature);

    function_signature *Print = Context->Functions + Context->FunctionCount++;
    ZeroStruct(*Print);
    Print->Name = StringFromC("print");
    Print->Label = "print";
    Print->IsBuiltin = 1;
    Print->Builtin = Builtin_Print;
    Print->Return = SimpleType(Type_Nil);
    Print->FirstParamSymbol = SymbolNone;

    function_signature *TypeFn = Context->Functions + Context->FunctionCount++;
    ZeroStruct(*TypeFn);
    TypeFn->Name = StringFromC("type");
    TypeFn->Label = "type";
    TypeFn->IsBuiltin = 1;
    TypeFn->Builtin = Builtin_Type;
    TypeFn->Return = SimpleType(Type_String);
    TypeFn->FirstParamSymbol = SymbolNone;

    function_signature *Chunk = Context->Functions + Context->FunctionCount++;
    ZeroStruct(*Chunk);
    Chunk->Name = StringFromC("lua_main");
    Chunk->Label = "lua_main";
    Chunk->Return = SimpleType(Type_Nil);
    Chunk->FirstParamSymbol = SymbolNone;

    Assert(Context->FunctionCount == ChunkFunctionIndex + 1);
}

internal void
AnalyzeExpr(semantic_context *Context, ASTIndex Index)
{
    Parser *P = Context->Parser;
    Expr *E = GetExpr(P, Index);

    switch(E->type)
    {
        case ExprNil:
            {
                E->Type = SimpleType(Type_Nil);
            } break;

        case ExprBoolean:
            {
                E->Type = SimpleType(Type_Bool);
            } break;

        case ExprNumber:
            {
                E->Type = SimpleType(Type_Number);
            } break;

        case ExprString:
            {
                E->Type = SimpleType(Type_String);
            } break;

        case ExprVariable:
            {
                string Name = StringFromC((char *)E->as.variable);
                semantic_symbol *Symbol = FindSymbol(Context, Name);
                if(Symbol)
                {
                    E->SymbolIndex = (u32)(Symbol - Context->Symbols);
                    E->Type = Symbol->Type;
                }
                else
            {
                    u32 BuiltinIndex = FindBuiltinSignature(Context, Name);
                    if(BuiltinIndex == SymbolNone)
                    {
                        SemanticError(Context, "undefined variable '%.*s'",
                                      (int)Name.Count, Name.Data);
                        E->Type = SimpleType(Type_Unknown);
                    }
                    else if(!E->IsCallCallee)
                    {
                        SemanticError(Context,
                                      "builtin '%.*s' may only be called directly",
                                      (int)Name.Count, Name.Data);
                        E->Type = SimpleType(Type_Unknown);
                    }
                    else
                {
                        E->Type = SimpleType(Type_Function);
                        E->Type.SignatureIndex = BuiltinIndex;
                    }
                }
            } break;

        case ExprGroup:
            {
                AnalyzeExpr(Context, E->as.group);
                E->Type = GetExpr(P, E->as.group)->Type;
            } break;

        case ExprUnary:
            {
                AnalyzeExpr(Context, E->as.unary.l_expr);
                switch(E->as.unary.op)
                {
                    case TK_NOT:
                        {
                            RequireExprType(Context, E->as.unary.l_expr,
                                            SimpleType(Type_Bool), "not");
                            E->Type = SimpleType(Type_Bool);
                        } break;

                    case TK_SUB:
                        {
                            RequireExprType(Context, E->as.unary.l_expr,
                                            SimpleType(Type_Number), "unary '-'");
                            E->Type = SimpleType(Type_Number);
                        } break;

                    case TK_HASH:
                        {
                            RequireExprType(Context, E->as.unary.l_expr,
                                            SimpleType(Type_String), "'#'");
                            E->Type = SimpleType(Type_Number);
                        } break;

                        InvalidDefaultCase;
                }
            } break;

        case ExprBinary:
            {
                ASTIndex LeftIndex = E->as.binary.l_expr;
                ASTIndex RightIndex = E->as.binary.r_expr;
                AnalyzeExpr(Context, LeftIndex);
                AnalyzeExpr(Context, RightIndex);
                ReservedToken Op = E->as.binary.op;

                if((Op == TK_AND) || (Op == TK_OR))
                {
                    RequireExprType(Context, LeftIndex, SimpleType(Type_Bool),
                                    "logical operator");
                    RequireExprType(Context, RightIndex, SimpleType(Type_Bool),
                                    "logical operator");
                    E->Type = SimpleType(Type_Bool);
                }
                else if((Op == TK_EQ_EQ) || (Op == TK_NEQ))
                {
                    UnifyExprType(Context, RightIndex,
                                  GetExpr(P, LeftIndex)->Type);
                    E->Type = SimpleType(Type_Bool);
                }
                else if((Op == TK_LT) || (Op == TK_LT_EQ) || (Op == TK_GT) ||
                    (Op == TK_GT_EQ))
                {
                    RequireExprType(Context, LeftIndex, SimpleType(Type_Number),
                                    "comparison");
                    RequireExprType(Context, RightIndex, SimpleType(Type_Number),
                                    "comparison");
                    E->Type = SimpleType(Type_Bool);
                }
                else
            {
                    RequireExprType(Context, LeftIndex, SimpleType(Type_Number),
                                    "arithmetic");
                    RequireExprType(Context, RightIndex, SimpleType(Type_Number),
                                    "arithmetic");
                    E->Type = SimpleType(Type_Number);
                }
            } break;

        case ExprFuncCall:
            {
                ASTIndex CalleeIndex = E->as.call.callee;
                Expr *Callee = GetExpr(P, CalleeIndex);
                if(Callee->type != ExprVariable)
                {
                    SemanticError(Context, "only direct function calls are supported");
                    E->Type = SimpleType(Type_Unknown);
                    break;
                }

                Callee->IsCallCallee = 1;
                AnalyzeExpr(Context, CalleeIndex);
                Callee->IsCallCallee = 0;
                if(Callee->Type.Kind != Type_Function)
                {
                    SemanticError(Context, "'%s' is not a function",
                                  TypeName(Callee->Type));
                    E->Type = SimpleType(Type_Unknown);
                    break;
                }

                function_signature *Sig =
                    GetFunction(Context, Callee->Type.SignatureIndex);
                u32 ArgCount = E->as.call.argc;

                if(!Sig->IsBuiltin && (ArgCount != Sig->ParamCount))
                {
                    SemanticError(Context,
                                  "wrong argument count for '%.*s': got %u, expected %u",
                                  (int)Sig->Name.Count, Sig->Name.Data, ArgCount,
                                  Sig->ParamCount);
                }

                ASTIndex Arg = E->as.call.i_arg;
                u32 ArgIndex = 0;
                for(; Arg != ASTNone; Arg = GetExpr(P, Arg)->next, ++ArgIndex)
                {
                    AnalyzeExpr(Context, Arg);
                    if(!Sig->IsBuiltin)
                    {
                        if(ArgIndex < Sig->ParamCount)
                        {
                            UnifyExprType(Context, Arg, Sig->Params[ArgIndex]);
                            Sig->Params[ArgIndex] = GetExpr(P, Arg)->Type;
                            if(Sig->FirstParamSymbol != SymbolNone)
                            {
                                Context->Symbols[Sig->FirstParamSymbol + ArgIndex]
                                    .Type = Sig->Params[ArgIndex];
                            }
                        }
                    }
                }

                E->Type = Sig->Return;
            } break;

            InvalidDefaultCase;
    }
}

internal void
AnalyzeStatement(semantic_context *Context, ASTIndex Index)
{
    Parser *P = Context->Parser;
    Statement *S = GetStatement(P, Index);

    switch(S->type)
    {
        case StmtBlock:
            {
                PushScope(Context);
                AnalyzeList(Context, S->as.block.block_idx);
                PopScope(Context);
            } break;

        case StmtLocalVar:
            {
                lua_type Type = SimpleType(Type_Unknown);
                if(S->as.local_var.value != ASTNone)
                {
                    AnalyzeExpr(Context, S->as.local_var.value);
                    Type = GetExpr(P, S->as.local_var.value)->Type;
                }
                string Name = StringFromC((char *)S->as.local_var.name);
                u32 SymbolIndex = DeclareLocal(Context, Name, Type);
                S->SymbolIndex = SymbolIndex;
                S->Type = Context->Symbols[SymbolIndex].Type;
            } break;

        case StmtAssignment:
            {
                ASTIndex ValueIndex = S->as.assign.value;
                AnalyzeExpr(Context, ValueIndex);
                lua_type ValueType = GetExpr(P, ValueIndex)->Type;
                string Name = StringFromC((char *)S->as.assign.name);

                semantic_symbol *Symbol = FindSymbol(Context, Name);
                if(Symbol)
                {
                    if(!UnifyType(&Symbol->Type, ValueType))
                    {
                        SemanticError(Context,
                                      "cannot assign %s to '%.*s' of type %s",
                                      TypeName(ValueType), (int)Name.Count,
                                      Name.Data, TypeName(Symbol->Type));
                    }
                    S->SymbolIndex = (u32)(Symbol - Context->Symbols);
                }
                else
            {
                    S->SymbolIndex = DeclareGlobal(Context, Name, ValueType);
                    Symbol = GetSymbol(Context, S->SymbolIndex);
                }
                S->Type = Symbol->Type;
            } break;

        case StmtExpr:
            {
                AnalyzeExpr(Context, S->as.expr_stmt);
            } break;

        case StmtFuncCall:
            {
                AnalyzeExpr(Context, S->as.funccall);
            } break;

        case StmtIfElse:
            {
                ASTIndex Current = Index;
                while(Current != ASTNone)
                {
                    Statement *If = GetStatement(P, Current);
                    if(If->as.if_stmt.cond != ASTNone)
                    {
                        AnalyzeExpr(Context, If->as.if_stmt.cond);
                        RequireExprType(Context, If->as.if_stmt.cond,
                                        SimpleType(Type_Bool), "if condition");
                    }
                    PushScope(Context);
                    AnalyzeList(Context, If->as.if_stmt.block_idx);
                    PopScope(Context);
                    Current = If->as.if_stmt.next_el;
                }
            } break;

        case StmtWhile:
            {
                AnalyzeExpr(Context, S->as.while_or_repeat_stmt.cond);
                RequireExprType(Context, S->as.while_or_repeat_stmt.cond,
                                SimpleType(Type_Bool), "while condition");
                ++Context->LoopDepth;
                PushScope(Context);
                AnalyzeList(Context, S->as.while_or_repeat_stmt.block_idx);
                PopScope(Context);
                --Context->LoopDepth;
            } break;

        case StmtRepeatUntil:
            {
                ++Context->LoopDepth;
                PushScope(Context);
                AnalyzeList(Context, S->as.while_or_repeat_stmt.block_idx);
                PopScope(Context);
                --Context->LoopDepth;
                AnalyzeExpr(Context, S->as.while_or_repeat_stmt.cond);
                RequireExprType(Context, S->as.while_or_repeat_stmt.cond,
                                SimpleType(Type_Bool), "until condition");
            } break;

        case StmtForLoop:
            {
                AnalyzeExpr(Context, S->as.for_stmt.start);
                AnalyzeExpr(Context, S->as.for_stmt.end_);
                RequireExprType(Context, S->as.for_stmt.start,
                                SimpleType(Type_Number), "numeric for");
                RequireExprType(Context, S->as.for_stmt.end_,
                                SimpleType(Type_Number), "numeric for");
                if(S->as.for_stmt.step != ASTNone)
                {
                    AnalyzeExpr(Context, S->as.for_stmt.step);
                    RequireExprType(Context, S->as.for_stmt.step,
                                    SimpleType(Type_Number), "numeric for step");
                }

                ++Context->LoopDepth;
                PushScope(Context);
                string Name = StringFromC((char *)S->as.for_stmt.var);
                S->SymbolIndex = DeclareLocal(Context, Name, SimpleType(Type_Number));
                AnalyzeList(Context, S->as.for_stmt.block_idx);
                PopScope(Context);
                --Context->LoopDepth;
            } break;

        case StmtFunction:
            {
                u32 SigIndex = S->Type.SignatureIndex;
                function_signature *Sig = GetFunction(Context, SigIndex);
                u32 SavedFunction = Context->CurrentFunction;
                Context->CurrentFunction = SigIndex;

                PushScope(Context);
                ASTIndex Param = S->as.func_stmt.first_arg;
                for(u32 ParamIndex = 0; ParamIndex < Sig->ParamCount; ++ParamIndex)
                {
                    string Name =
                        StringFromC((char *)GetExpr(P, Param)->as.variable);
                    u32 SymbolIndex = DeclareLocal(Context, Name,
                                                   Sig->Params[ParamIndex]);
                    if(ParamIndex == 0)
                    {
                        Sig->FirstParamSymbol = SymbolIndex;
                    }
                    Param = GetExpr(P, Param)->next;
                }

                AnalyzeList(Context, S->as.func_stmt.first_stmt);

                /* The body constrains parameter types through their symbols.
                 * Copy that result back so later calls are checked against the
                 * body's requirements, not against a stale Unknown. */
                if(Sig->FirstParamSymbol != SymbolNone)
                {
                    for(u32 ParamIndex = 0; ParamIndex < Sig->ParamCount;
                        ++ParamIndex)
                    {
                        Sig->Params[ParamIndex] =
                            Context->Symbols[Sig->FirstParamSymbol + ParamIndex]
                                .Type;
                    }
                }

                if(TypeIsUnknown(Sig->Return))
                {
                    Sig->Return = SimpleType(Type_Nil);
                }
                PopScope(Context);
                Context->CurrentFunction = SavedFunction;
            } break;

        case StmtReturn:
            {
                if(Context->CurrentFunction == ChunkFunctionIndex)
                {
                    SemanticError(Context, "'return' is not allowed in the main chunk");
                    break;
                }
                function_signature *Sig =
                    GetFunction(Context, Context->CurrentFunction);
                if(S->as.return_value != ASTNone)
                {
                    AnalyzeExpr(Context, S->as.return_value);
                    lua_type ValueType = GetExpr(P, S->as.return_value)->Type;
                    if(!UnifyType(&Sig->Return, ValueType))
                    {
                        SemanticError(Context,
                                      "cannot return %s from function returning %s",
                                      TypeName(ValueType), TypeName(Sig->Return));
                    }
                }
                else
                {
                    lua_type NilType = SimpleType(Type_Nil);
                    if(!UnifyType(&Sig->Return, NilType))
                    {
                        SemanticError(Context,
                                      "function returning %s cannot return no value",
                                      TypeName(Sig->Return));
                    }
                }
            } break;

        case StmtBreak:
            {
                if(Context->LoopDepth == 0)
                {
                    SemanticError(Context, "'break' is only valid inside a loop");
                }
            } break;

            InvalidDefaultCase;
    }
}

internal void
AnalyzeList(semantic_context *Context, ASTIndex First)
{
    Parser *P = Context->Parser;
    for(ASTIndex Index = First; Index != ASTNone;)
    {
        AnalyzeStatement(Context, Index);
        Index = GetStatement(P, Index)->next;
    }
}

internal void
FinalizeUnknownTypes(semantic_context *Context)
{
    Parser *P = Context->Parser;

    for(u32 Index = 0; Index < Context->SymbolCount; ++Index)
    {
        if(TypeIsUnknown(Context->Symbols[Index].Type))
        {
            Context->Symbols[Index].Type = SimpleType(Type_Nil);
        }
    }

    for(u32 FuncIndex = 0; FuncIndex < Context->FunctionCount; ++FuncIndex)
    {
        function_signature *Sig = GetFunction(Context, FuncIndex);
        for(u32 ParamIndex = 0; ParamIndex < Sig->ParamCount; ++ParamIndex)
        {
            if(TypeIsUnknown(Sig->Params[ParamIndex]))
            {
                Sig->Params[ParamIndex] = SimpleType(Type_Nil);
            }
        }
        if(TypeIsUnknown(Sig->Return))
        {
            Sig->Return = SimpleType(Type_Nil);
        }
    }

    for(u32 Index = 0; Index < P->ExprLen; ++Index)
    {
        if(TypeIsUnknown(P->ExprPool[Index].Type))
        {
            P->ExprPool[Index].Type = SimpleType(Type_Nil);
        }
    }

    for(u32 Index = 0; Index < P->StmtLen; ++Index)
    {
        if(TypeIsUnknown(P->StmtPool[Index].Type))
        {
            P->StmtPool[Index].Type = SimpleType(Type_Nil);
        }
    }
}

void
AnalyzeSemantic(semantic_context *Context, ASTIndex FirstStatement)
{
    RegisterFunctionList(Context, FirstStatement);
    PushScope(Context);
    AnalyzeList(Context, FirstStatement);
    PopScope(Context);
    FinalizeUnknownTypes(Context);
}
