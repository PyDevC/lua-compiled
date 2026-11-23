/* ========================================================================

   $File: ir.c
   $Date: 2026/11/20
   $Revision: 1

   Lowers the annotated tree to typed three-address code. Every expression
   produces one virtual register; every variable reference is already a symbol
   slot from the semantic pass. There is no interpreter here.

   ======================================================================== */

#include "ir.h"

internal ir_operand
IRNoOperand(void)
{
    ir_operand Result = {0};
    Result.Kind = IR_None;
    return(Result);
}

internal ir_operand
IRRegOperand(u32 Reg)
{
    ir_operand Result = {0};
    Result.Kind = IR_Reg;
    Result.Reg = Reg;
    return(Result);
}

internal ir_operand
IRSymbolOperand(u32 SymbolIndex)
{
    ir_operand Result = {0};
    Result.Kind = IR_Symbol;
    Result.SymbolIndex = SymbolIndex;
    return(Result);
}

internal ir_operand
IRFunctionOperand(u32 FunctionIndex)
{
    ir_operand Result = {0};
    Result.Kind = IR_Function;
    Result.FunctionIndex = FunctionIndex;
    return(Result);
}

internal ir_operand
IRConstOperand(ir_constant Constant)
{
    ir_operand Result = {0};
    Result.Kind = IR_Const;
    Result.Const = Constant;
    return(Result);
}

internal ir_constant
NumberConst(f64 Number)
{
    ir_constant Result = {0};
    Result.Type = SimpleType(Type_Number);
    Result.Number = Number;
    return(Result);
}

internal ir_constant
BoolConst(b32 Boolean)
{
    ir_constant Result = {0};
    Result.Type = SimpleType(Type_Bool);
    Result.Boolean = Boolean;
    return(Result);
}

internal ir_constant
NilConst(void)
{
    ir_constant Result = {0};
    Result.Type = SimpleType(Type_Nil);
    return(Result);
}

internal ir_constant
StringConst(string String)
{
    ir_constant Result = {0};
    Result.Type = SimpleType(Type_String);
    Result.String = String;
    return(Result);
}

internal u32
NextReg(ir_program *IR, u32 Count)
{
    u32 Result = IR->FrameRegCount;
    IR->FrameRegCount += Count;
    Assert(IR->FrameRegCount <= IR->RegTypeCapacity);
    return(Result);
}

internal b32
ExprUsesSymbol(ir_program *IR, ASTIndex Index, u32 SymbolIndex)
{
    b32 Result = 0;
    if(Index == ASTNone)
    {
        return(Result);
    }

    Expr *E = GetExpr(IR->Parser, Index);
    if((E->type == ExprVariable) && (E->SymbolIndex == SymbolIndex))
    {
        Result = 1;
    }

    switch(E->type)
    {
        case ExprNil:
        case ExprBoolean:
        case ExprNumber:
        case ExprString:
        case ExprVariable:
        {
        } break;

        case ExprUnary:
        {
            Result = Result || ExprUsesSymbol(IR, E->as.unary.l_expr,
                                              SymbolIndex);
        } break;

        case ExprBinary:
        {
            Result = Result || ExprUsesSymbol(IR, E->as.binary.l_expr,
                                              SymbolIndex);
            Result = Result || ExprUsesSymbol(IR, E->as.binary.r_expr,
                                              SymbolIndex);
        } break;

        case ExprGroup:
        {
            Result = Result || ExprUsesSymbol(IR, E->as.group, SymbolIndex);
        } break;

        case ExprFuncCall:
        {
            Result = Result || ExprUsesSymbol(IR, E->as.call.callee,
                                              SymbolIndex);
            Result = Result || ExprUsesSymbol(IR, E->as.call.i_arg,
                                              SymbolIndex);
        } break;

        InvalidDefaultCase;
    }

    if(E->next != ASTNone)
    {
        Result = Result || ExprUsesSymbol(IR, E->next, SymbolIndex);
    }
    return(Result);
}

internal b32
StatementsUseSymbol(ir_program *IR, ASTIndex Index, u32 SymbolIndex)
{
    b32 Result = 0;
    while(Index != ASTNone)
    {
        Statement *S = GetStatement(IR->Parser, Index);
        if((S->type == StmtAssignment) && (S->SymbolIndex == SymbolIndex))
        {
            Result = 1;
        }

        switch(S->type)
        {
            case StmtBlock:
            {
                Result = Result || StatementsUseSymbol(IR, S->as.block.block_idx,
                                                       SymbolIndex);
            } break;

            case StmtLocalVar:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.local_var.value,
                                                  SymbolIndex);
            } break;

            case StmtAssignment:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.assign.value,
                                                  SymbolIndex);
            } break;

            case StmtIfElse:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.if_stmt.cond,
                                                  SymbolIndex);
                Result = Result || StatementsUseSymbol(IR,
                                    S->as.if_stmt.block_idx, SymbolIndex);
                Result = Result || StatementsUseSymbol(IR,
                                    S->as.if_stmt.next_el, SymbolIndex);
            } break;

            case StmtWhile:
            case StmtRepeatUntil:
            {
                Result = Result || ExprUsesSymbol(IR,
                    S->as.while_or_repeat_stmt.cond, SymbolIndex);
                Result = Result || StatementsUseSymbol(IR,
                    S->as.while_or_repeat_stmt.block_idx, SymbolIndex);
            } break;

            case StmtForLoop:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.for_stmt.start,
                                                  SymbolIndex);
                Result = Result || ExprUsesSymbol(IR, S->as.for_stmt.end_,
                                                  SymbolIndex);
                Result = Result || ExprUsesSymbol(IR, S->as.for_stmt.step,
                                                  SymbolIndex);
                Result = Result || StatementsUseSymbol(IR,
                    S->as.for_stmt.block_idx, SymbolIndex);
            } break;

            case StmtFuncCall:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.funccall,
                                                  SymbolIndex);
            } break;

            case StmtFunction:
            {
                Result = Result || ExprUsesSymbol(IR,
                    S->as.func_stmt.first_arg, SymbolIndex);
                Result = Result || StatementsUseSymbol(IR,
                    S->as.func_stmt.first_stmt, SymbolIndex);
            } break;

            case StmtExpr:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.expr_stmt,
                                                  SymbolIndex);
            } break;

            case StmtReturn:
            {
                Result = Result || ExprUsesSymbol(IR, S->as.return_value,
                                                  SymbolIndex);
            } break;

            case StmtBreak:
            {
            } break;

            InvalidDefaultCase;
        }

        Index = S->next;
    }
    return(Result);
}

internal lua_type
GetRegType(ir_program *IR, u32 FunctionIndex, u32 Reg)
{
    lua_type Result = SimpleType(Type_Unknown);
    if(Reg != IRInvalid)
    {
        for(u32 Index = IR->RegTypeCount; Index > 0; --Index)
        {
            ir_reg_type *Entry = IR->RegTypes + (Index - 1);
            if((Entry->FunctionIndex == FunctionIndex) && (Entry->Reg == Reg))
            {
                Result = Entry->Type;
                break;
            }
        }
    }
    return(Result);
}

internal void
SetRegType(ir_program *IR, u32 Reg, lua_type Type)
{
    if(Reg != IRInvalid)
    {
        Assert(IR->RegTypeCount < IR->RegTypeCapacity);
        ir_reg_type *Entry = IR->RegTypes + IR->RegTypeCount++;
        Entry->FunctionIndex = IR->CurrentFunction;
        Entry->Reg = Reg;
        Entry->Type = Type;
    }
}

internal void
SetOperandType(ir_program *IR, ir_operand Operand, lua_type Type)
{
    if(Operand.Kind == IR_Reg)
    {
        SetRegType(IR, Operand.Reg, Type);
    }
}

internal u32
EmitIR(ir_program *IR, ir_op Op, lua_type Type, ir_operand Dst,
       ir_operand A, ir_operand B)
{
    Assert(IR->InstructionCount < IR->InstructionCapacity);
    u32 Index = IR->InstructionCount++;
    ir_instruction *Instr = IR->Instructions + Index;
    ZeroStruct(*Instr);
    Instr->Op = Op;
    Instr->Type = Type;
    Instr->Dst = Dst;
    Instr->A = A;
    Instr->B = B;
    Instr->Target = IRInvalid;
    Instr->OwnerFunction = IR->CurrentFunction;
    Instr->CalleeIndex = IRInvalid;

    switch(Op)
    {
        case IR_LoadConst:
        case IR_LoadFunction:
        case IR_LoadSymbol:
        {
            SetOperandType(IR, Dst, Type);
        } break;

        case IR_StoreSymbol:
        case IR_Return:
        {
            SetOperandType(IR, B, Type);
            SetOperandType(IR, A, Type);
        } break;

        case IR_Move:
        case IR_Add:
        case IR_Sub:
        case IR_Mul:
        case IR_Div:
        case IR_Neg:
        case IR_Not:
        {
            SetOperandType(IR, Dst, Type);
            SetOperandType(IR, A, Type);
            SetOperandType(IR, B, Type);
        } break;

        case IR_Eq:
        case IR_Neq:
        case IR_Lt:
        case IR_Le:
        case IR_Gt:
        case IR_Ge:
        {
            /* Only the result is bool; the operands keep their own type. */
            SetOperandType(IR, Dst, Type);
        } break;

        case IR_Call:
        {
            SetOperandType(IR, Dst, Type);
        } break;

        case IR_Len:
        case IR_Jump:
        case IR_JumpIfFalse:
        case IR_JumpIfTrue:
        {
            /* Operand types come from the producer; nothing to annotate. */
        } break;

        InvalidDefaultCase;
    }

    /* IR_Len reads a string and produces a number; IR_JumpIf* read booleans.
     * Their operand types are set by the producer, so leave them alone. */
    (void)Op;
    return(Index);
}

internal void
PatchInstruction(ir_program *IR, u32 Index, u32 Target)
{
    Assert(Index < IR->InstructionCount);
    IR->Instructions[Index].Target = Target;
}

internal u32
BeginPatches(ir_program *IR)
{
    return(IR->PatchCount);
}

internal void
AddPatch(ir_program *IR, u32 InstructionIndex)
{
    Assert(IR->PatchCount < IR->PatchCapacity);
    IR->Patches[IR->PatchCount++] = InstructionIndex;
}

internal void
PatchTo(ir_program *IR, u32 Start, u32 Target)
{
    for(u32 Index = Start; Index < IR->PatchCount; ++Index)
    {
        PatchInstruction(IR, IR->Patches[Index], Target);
    }
    IR->PatchCount = Start;
}

internal u32
BeginBreakPatches(ir_program *IR)
{
    return(IR->BreakCount);
}

internal void
AddBreakPatch(ir_program *IR, u32 InstructionIndex)
{
    Assert(IR->BreakCount < IR->BreakCapacity);
    IR->BreakPatches[IR->BreakCount++] = InstructionIndex;
}

internal void
PatchBreaksTo(ir_program *IR, u32 Start, u32 Target)
{
    for(u32 Index = Start; Index < IR->BreakCount; ++Index)
    {
        PatchInstruction(IR, IR->BreakPatches[Index], Target);
    }
    IR->BreakCount = Start;
}

internal void
PushLoopPatches(ir_program *IR, u32 Start)
{
    Assert(IR->LoopCount < IR->LoopCapacity);
    IR->LoopPatchStack[IR->LoopCount++] = Start;
}

internal void
PopLoopPatches(ir_program *IR)
{
    Assert(IR->LoopCount > 0);
    --IR->LoopCount;
}

internal u32
EmitExpr(ir_program *IR, ASTIndex Index);

internal u32
EmitCall(ir_program *IR, Expr *E, u32 TargetReg);

internal void
EmitExprInto(ir_program *IR, ASTIndex Index, u32 TargetReg)
{
    Parser *P = IR->Parser;
    Expr *E = GetExpr(P, Index);

    /* Simple producers can write straight into the caller's register. That
     * removes the load/move pairs that used to surround every call argument. */
    switch(E->type)
    {
        case ExprNil:
        {
            EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(TargetReg),
                   IRConstOperand(NilConst()), IRNoOperand());
            return;
        } break;

        case ExprBoolean:
        {
            EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(TargetReg),
                   IRConstOperand(BoolConst(E->as.boolean)), IRNoOperand());
            return;
        } break;

        case ExprNumber:
        {
            EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(TargetReg),
                   IRConstOperand(NumberConst(E->as.num)), IRNoOperand());
            return;
        } break;

        case ExprString:
        {
            EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(TargetReg),
                   IRConstOperand(StringConst(StringFromC((char *)E->as.string))),
                   IRNoOperand());
            return;
        } break;

        case ExprVariable:
        {
            EmitIR(IR, IR_LoadSymbol, E->Type, IRRegOperand(TargetReg),
                   IRSymbolOperand(E->SymbolIndex), IRNoOperand());
            return;
        } break;

        case ExprGroup:
        {
            EmitExprInto(IR, E->as.group, TargetReg);
            return;
        } break;

        case ExprFuncCall:
        {
            EmitCall(IR, E, TargetReg);
            return;
        } break;

        case ExprUnary:
        case ExprBinary:
        default:
        {
            /* Complex expressions still need a temporary register first. */
        } break;
    }

    u32 SourceReg = EmitExpr(IR, Index);
    if(SourceReg != TargetReg)
    {
        EmitIR(IR, IR_Move, E->Type, IRRegOperand(TargetReg),
               IRRegOperand(SourceReg), IRNoOperand());
    }
}

internal u32
EmitBinary(ir_program *IR, ir_op Op, lua_type ResultType, u32 Left, u32 Right)
{
    u32 Result = NextReg(IR, 1);
    EmitIR(IR, Op, ResultType, IRRegOperand(Result), IRRegOperand(Left),
           IRRegOperand(Right));
    return(Result);
}

internal void
EmitBinaryInto(ir_program *IR, ir_op Op, lua_type ResultType, u32 Left,
               u32 Right, u32 TargetReg)
{
    EmitIR(IR, Op, ResultType, IRRegOperand(TargetReg), IRRegOperand(Left),
           IRRegOperand(Right));
}

internal u32
EmitNumberReg(ir_program *IR, f64 Number)
{
    u32 Result = NextReg(IR, 1);
    EmitIR(IR, IR_LoadConst, SimpleType(Type_Number), IRRegOperand(Result),
           IRConstOperand(NumberConst(Number)), IRNoOperand());
    return(Result);
}

internal u32
EmitCall(ir_program *IR, Expr *E, u32 TargetReg)
{
    Parser *P = IR->Parser;
    Expr *Callee = GetExpr(P, E->as.call.callee);
    Assert(Callee->type == ExprVariable);
    u32 FunctionIndex = Callee->Type.SignatureIndex;

    u32 ArgBase = NextReg(IR, E->as.call.argc);
    ASTIndex Arg = E->as.call.i_arg;
    for(u32 ArgIndex = 0; ArgIndex < E->as.call.argc; ++ArgIndex)
    {
        EmitExprInto(IR, Arg, ArgBase + ArgIndex);
        Arg = GetExpr(P, Arg)->next;
    }

    if(TargetReg == IRInvalid)
    {
        TargetReg = NextReg(IR, 1);
    }
    u32 CallIndex = EmitIR(IR, IR_Call, E->Type, IRRegOperand(TargetReg),
                           IRNoOperand(), IRNoOperand());
    IR->Instructions[CallIndex].CalleeIndex = FunctionIndex;
    IR->Instructions[CallIndex].ArgBase = ArgBase;
    IR->Instructions[CallIndex].ArgCount = E->as.call.argc;
    return(TargetReg);
}

internal u32
EmitExpr(ir_program *IR, ASTIndex Index)
{
    Parser *P = IR->Parser;
    Expr *E = GetExpr(P, Index);
    u32 Result = IRInvalid;

    switch(E->type)
    {
        case ExprNil:
            {
                Result = NextReg(IR, 1);
                EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(Result),
                       IRConstOperand(NilConst()), IRNoOperand());
            } break;

        case ExprBoolean:
            {
                Result = NextReg(IR, 1);
                EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(Result),
                       IRConstOperand(BoolConst(E->as.boolean)), IRNoOperand());
            } break;

        case ExprNumber:
            {
                Result = NextReg(IR, 1);
                EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(Result),
                       IRConstOperand(NumberConst(E->as.num)), IRNoOperand());
            } break;

        case ExprString:
            {
                Result = NextReg(IR, 1);
                EmitIR(IR, IR_LoadConst, E->Type, IRRegOperand(Result),
                       IRConstOperand(StringConst(StringFromC((char *)E->as.string))),
                       IRNoOperand());
            } break;

        case ExprVariable:
            {
                Result = NextReg(IR, 1);
                EmitIR(IR, IR_LoadSymbol, E->Type, IRRegOperand(Result),
                       IRSymbolOperand(E->SymbolIndex), IRNoOperand());
            } break;

        case ExprGroup:
            {
                Result = EmitExpr(IR, E->as.group);
            } break;

        case ExprUnary:
            {
                u32 Operand = EmitExpr(IR, E->as.unary.l_expr);
                Result = NextReg(IR, 1);
                ir_op Op = IR_Not;
                switch(E->as.unary.op)
                {
                    case TK_NOT: { Op = IR_Not; } break;
                    case TK_SUB: { Op = IR_Neg; } break;
                    case TK_HASH: { Op = IR_Len; } break;
                        InvalidDefaultCase;
                }
                EmitIR(IR, Op, E->Type, IRRegOperand(Result),
                       IRRegOperand(Operand), IRNoOperand());
            } break;

        case ExprBinary:
            {
                ReservedToken TokenOp = E->as.binary.op;

                if((TokenOp == TK_AND) || (TokenOp == TK_OR))
                {
                    u32 Left = EmitExpr(IR, E->as.binary.l_expr);
                    u32 JumpIndex = EmitIR(
                        IR, (TokenOp == TK_AND) ? IR_JumpIfFalse : IR_JumpIfTrue,
                        E->Type, IRNoOperand(), IRRegOperand(Left), IRNoOperand());
                    EmitExprInto(IR, E->as.binary.r_expr, Left);
                    PatchInstruction(IR, JumpIndex, IR->InstructionCount);
                    Result = Left;
                }
                else
            {
                    u32 Left = EmitExpr(IR, E->as.binary.l_expr);
                    u32 Right = EmitExpr(IR, E->as.binary.r_expr);
                    ir_op Op = IR_Add;
                    switch(TokenOp)
                    {
                        case TK_ADD:    { Op = IR_Add; } break;
                        case TK_SUB:    { Op = IR_Sub; } break;
                        case TK_MUL:    { Op = IR_Mul; } break;
                        case TK_DIV:    { Op = IR_Div; } break;
                        case TK_EQ_EQ:  { Op = IR_Eq; } break;
                        case TK_NEQ:    { Op = IR_Neq; } break;
                        case TK_LT:     { Op = IR_Lt; } break;
                        case TK_LT_EQ:  { Op = IR_Le; } break;
                        case TK_GT:     { Op = IR_Gt; } break;
                        case TK_GT_EQ:  { Op = IR_Ge; } break;
                            InvalidDefaultCase;
                    }
                    Result = EmitBinary(IR, Op, E->Type, Left, Right);
                }
            } break;

        case ExprFuncCall:
            {
                Result = EmitCall(IR, E, IRInvalid);
            } break;

            InvalidDefaultCase;
    }

    return(Result);
}

internal void EmitStatements(ir_program *IR, ASTIndex First);

internal void
EmitStatement(ir_program *IR, ASTIndex Index)
{
    Parser *P = IR->Parser;
    Statement *S = GetStatement(P, Index);

    switch(S->type)
    {
        case StmtBlock:
            {
                EmitStatements(IR, S->as.block.block_idx);
            } break;

        case StmtLocalVar:
            {
                u32 Reg = NextReg(IR, 1);
                if(S->as.local_var.value != ASTNone)
                {
                    EmitExprInto(IR, S->as.local_var.value, Reg);
                }
                else
            {
                    EmitIR(IR, IR_LoadConst, S->Type, IRRegOperand(Reg),
                           IRConstOperand(NilConst()), IRNoOperand());
                }
                EmitIR(IR, IR_StoreSymbol, S->Type, IRNoOperand(),
                       IRSymbolOperand(S->SymbolIndex), IRRegOperand(Reg));
            } break;

        case StmtAssignment:
            {
                u32 Reg = EmitExpr(IR, S->as.assign.value);
                EmitIR(IR, IR_StoreSymbol, S->Type, IRNoOperand(),
                       IRSymbolOperand(S->SymbolIndex), IRRegOperand(Reg));
            } break;

        case StmtExpr:
            {
                EmitExpr(IR, S->as.expr_stmt);
            } break;

        case StmtFuncCall:
            {
                EmitExpr(IR, S->as.funccall);
            } break;

        case StmtIfElse:
            {
                u32 ExitPatches = BeginPatches(IR);
                ASTIndex Current = Index;
                while(Current != ASTNone)
                {
                    Statement *If = GetStatement(P, Current);
                    if(If->as.if_stmt.cond != ASTNone)
                    {
                        u32 CondReg = EmitExpr(IR, If->as.if_stmt.cond);
                        u32 FalseJump = EmitIR(IR, IR_JumpIfFalse, SimpleType(Type_Nil),
                                               IRNoOperand(), IRRegOperand(CondReg),
                                               IRNoOperand());

                        EmitStatements(IR, If->as.if_stmt.block_idx);

                        if(If->as.if_stmt.next_el != ASTNone)
                        {
                            AddPatch(IR,
                                     EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                                            IRNoOperand(), IRNoOperand(),
                                            IRNoOperand()));
                        }
                        PatchInstruction(IR, FalseJump, IR->InstructionCount);
                    }
                    else
                {
                        EmitStatements(IR, If->as.if_stmt.block_idx);
                    }
                    Current = If->as.if_stmt.next_el;
                }
                PatchTo(IR, ExitPatches, IR->InstructionCount);
            } break;

        case StmtWhile:
            {
                u32 LoopStart = IR->InstructionCount;
                u32 CondStart = BeginPatches(IR);
                u32 BreakStart = BeginBreakPatches(IR);
                PushLoopPatches(IR, BreakStart);

                u32 CondReg = EmitExpr(IR, S->as.while_or_repeat_stmt.cond);
                AddPatch(IR, EmitIR(IR, IR_JumpIfFalse, SimpleType(Type_Nil),
                                    IRNoOperand(), IRRegOperand(CondReg),
                                    IRNoOperand()));

                EmitStatements(IR, S->as.while_or_repeat_stmt.block_idx);
                u32 BackJump = EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                                      IRNoOperand(), IRNoOperand(), IRNoOperand());
                PatchInstruction(IR, BackJump, LoopStart);

                PopLoopPatches(IR);
                PatchTo(IR, CondStart, IR->InstructionCount);
                PatchBreaksTo(IR, BreakStart, IR->InstructionCount);
            } break;

        case StmtRepeatUntil:
            {
                u32 LoopStart = IR->InstructionCount;
                u32 CondStart = BeginPatches(IR);
                u32 BreakStart = BeginBreakPatches(IR);
                PushLoopPatches(IR, BreakStart);

                EmitStatements(IR, S->as.while_or_repeat_stmt.block_idx);
                u32 CondReg = EmitExpr(IR, S->as.while_or_repeat_stmt.cond);
                AddPatch(IR, EmitIR(IR, IR_JumpIfTrue, SimpleType(Type_Nil),
                                    IRNoOperand(), IRRegOperand(CondReg),
                                    IRNoOperand()));
                u32 BackJump = EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                                      IRNoOperand(), IRNoOperand(), IRNoOperand());
                PatchInstruction(IR, BackJump, LoopStart);

                PopLoopPatches(IR);
                PatchTo(IR, CondStart, IR->InstructionCount);
                PatchBreaksTo(IR, BreakStart, IR->InstructionCount);
            } break;

        case StmtForLoop:
            {
                u32 StartReg = EmitExpr(IR, S->as.for_stmt.start);
                u32 EndReg = EmitExpr(IR, S->as.for_stmt.end_);
                u32 StepReg = NextReg(IR, 1);
                if(S->as.for_stmt.step != ASTNone)
                {
                    EmitExprInto(IR, S->as.for_stmt.step, StepReg);
                }
                else
            {
                    EmitIR(IR, IR_LoadConst, SimpleType(Type_Number),
                           IRRegOperand(StepReg), IRConstOperand(NumberConst(1)),
                           IRNoOperand());
                }

                /* The loop counter lives in StartReg for the whole loop. It is
                 * only spilled to the symbol slot when the body can observe it;
                 * an empty loop then has no memory traffic at all. */
                b32 NeedStore = StatementsUseSymbol(IR,
                    S->as.for_stmt.block_idx, S->SymbolIndex);
                u32 CurrentReg = StartReg;

                u32 CondStart = BeginPatches(IR);
                u32 BreakStart = BeginBreakPatches(IR);
                PushLoopPatches(IR, BreakStart);
                u32 LoopStart = IR->InstructionCount;

                u32 ZeroReg = EmitNumberReg(IR, 0);
                u32 StepPositive = EmitBinary(IR, IR_Gt, SimpleType(Type_Bool),
                                              StepReg, ZeroReg);
                u32 NegativeCheck = EmitIR(IR, IR_JumpIfFalse, SimpleType(Type_Nil),
                                           IRNoOperand(), IRRegOperand(StepPositive),
                                           IRNoOperand());

                u32 DoneReg = NextReg(IR, 1);
                EmitBinaryInto(IR, IR_Gt, SimpleType(Type_Bool), CurrentReg, EndReg,
                               DoneReg);
                u32 JumpAfter = EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                                       IRNoOperand(), IRNoOperand(), IRNoOperand());
                PatchInstruction(IR, NegativeCheck, IR->InstructionCount);
                EmitBinaryInto(IR, IR_Lt, SimpleType(Type_Bool), CurrentReg, EndReg,
                               DoneReg);
                PatchInstruction(IR, JumpAfter, IR->InstructionCount);

                AddPatch(IR, EmitIR(IR, IR_JumpIfTrue, SimpleType(Type_Nil),
                                    IRNoOperand(), IRRegOperand(DoneReg),
                                    IRNoOperand()));

                if(NeedStore)
                {
                    EmitIR(IR, IR_StoreSymbol, SimpleType(Type_Number), IRNoOperand(),
                           IRSymbolOperand(S->SymbolIndex), IRRegOperand(CurrentReg));
                }

                EmitStatements(IR, S->as.for_stmt.block_idx);

                EmitBinaryInto(IR, IR_Add, SimpleType(Type_Number), CurrentReg,
                               StepReg, CurrentReg);
                u32 BackJump = EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                                      IRNoOperand(), IRNoOperand(), IRNoOperand());
                PatchInstruction(IR, BackJump, LoopStart);

                PopLoopPatches(IR);
                PatchTo(IR, CondStart, IR->InstructionCount);
                PatchBreaksTo(IR, BreakStart, IR->InstructionCount);
            } break;

        case StmtFunction:
            {
                u32 SigIndex = S->Type.SignatureIndex;
                u32 SkipJump = EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                                      IRNoOperand(), IRNoOperand(), IRNoOperand());

                ir_function_info *Info = IR->Functions + SigIndex;
                Info->Entry = IR->InstructionCount;
                Info->Emitted = 1;

                u32 SavedRegCount = IR->FrameRegCount;
                u32 SavedFunction = IR->CurrentFunction;
                IR->FrameRegCount = 0;
                IR->CurrentFunction = SigIndex;

                EmitStatements(IR, S->as.func_stmt.first_stmt);

                function_signature *Sig =
                    GetFunction(IR->Semantic, SigIndex);
                u32 ImplicitReg = NextReg(IR, 1);
                ir_constant Implicit = NilConst();
                if(Sig->Return.Kind == Type_Number)
                {
                    Implicit = NumberConst(0);
                }
                else if(Sig->Return.Kind == Type_Bool)
                {
                    Implicit = BoolConst(0);
                }
                EmitIR(IR, IR_LoadConst, Sig->Return, IRRegOperand(ImplicitReg),
                       IRConstOperand(Implicit), IRNoOperand());
                EmitIR(IR, IR_Return, Sig->Return, IRNoOperand(),
                       IRRegOperand(ImplicitReg), IRNoOperand());

                Info->RegCount = IR->FrameRegCount;
                PatchInstruction(IR, SkipJump, IR->InstructionCount);
                IR->FrameRegCount = SavedRegCount;
                IR->CurrentFunction = SavedFunction;

                u32 FunctionReg = NextReg(IR, 1);
                lua_type FunctionType = SimpleType(Type_Function);
                FunctionType.SignatureIndex = SigIndex;
                EmitIR(IR, IR_LoadFunction, FunctionType, IRRegOperand(FunctionReg),
                       IRFunctionOperand(SigIndex), IRNoOperand());
                EmitIR(IR, IR_StoreSymbol, FunctionType, IRNoOperand(),
                       IRSymbolOperand(S->SymbolIndex), IRRegOperand(FunctionReg));
            } break;

        case StmtReturn:
            {
                if(S->as.return_value == ASTNone)
                {
                    EmitIR(IR, IR_Return, SimpleType(Type_Nil), IRNoOperand(),
                           IRNoOperand(), IRNoOperand());
                }
                else
            {
                    u32 Reg = EmitExpr(IR, S->as.return_value);
                    EmitIR(IR, IR_Return, GetExpr(P, S->as.return_value)->Type,
                           IRNoOperand(), IRRegOperand(Reg), IRNoOperand());
                }
            } break;

        case StmtBreak:
            {
                Assert(IR->LoopCount > 0);
                AddBreakPatch(
                    IR, EmitIR(IR, IR_Jump, SimpleType(Type_Nil),
                               IRNoOperand(), IRNoOperand(), IRNoOperand()));
            } break;

            InvalidDefaultCase;
    }
}

internal void
EmitStatements(ir_program *IR, ASTIndex First)
{
    Parser *P = IR->Parser;
    for(ASTIndex Index = First; Index != ASTNone;)
    {
        EmitStatement(IR, Index);
        Index = GetStatement(P, Index)->next;
    }
}

internal void
InitIR(ir_program *IR, Parser *P, semantic_context *Semantic,
       memory_arena *Arena)
{
    ZeroStruct(*IR);
    IR->Parser = P;
    IR->Semantic = Semantic;
    IR->Arena = Arena;

    u32 NodeCount = P->StmtCap + P->ExprCap + 16;
    IR->InstructionCapacity = NodeCount * 4;
    IR->PatchCapacity = NodeCount * 2;
    IR->BreakCapacity = IR->PatchCapacity;
    IR->LoopCapacity = P->StmtCap + 16;
    IR->RegTypeCapacity = NodeCount * 16;
    IR->FunctionCount = Semantic->FunctionCount;

    IR->Instructions = PushArray(Arena, IR->InstructionCapacity, ir_instruction);
    IR->Patches = PushArray(Arena, IR->PatchCapacity, u32);
    IR->BreakPatches = PushArray(Arena, IR->BreakCapacity, u32);
    IR->LoopPatchStack = PushArray(Arena, IR->LoopCapacity, u32);
    IR->RegTypes = PushArray(Arena, IR->RegTypeCapacity, ir_reg_type);
    IR->Functions = PushArray(Arena, IR->FunctionCount, ir_function_info);
    for(u32 Index = 0; Index < IR->FunctionCount; ++Index)
    {
        IR->Functions[Index].Entry = IRInvalid;
        IR->Functions[Index].SignatureIndex = Index;
    }

    IR->CurrentFunction = ChunkFunctionIndex;
    IR->Functions[ChunkFunctionIndex].Entry = 0;
}

internal void
GenerateIR(ir_program *IR, ASTIndex FirstStatement)
{
    EmitStatements(IR, FirstStatement);

    u32 NilReg = NextReg(IR, 1);
    EmitIR(IR, IR_LoadConst, SimpleType(Type_Nil), IRRegOperand(NilReg),
           IRConstOperand(NilConst()), IRNoOperand());
    EmitIR(IR, IR_Return, SimpleType(Type_Nil), IRNoOperand(),
           IRRegOperand(NilReg), IRNoOperand());
    IR->Functions[ChunkFunctionIndex].RegCount = IR->FrameRegCount;

    /* Function bodies were emitted inline behind skip jumps. Patch any jump
     * that targeted the next function to the real end of the chunk. */
    PatchTo(IR, 0, IR->InstructionCount);
    PatchBreaksTo(IR, 0, IR->InstructionCount);
}

typedef struct ir_reg_def
{
    u32 Count;
    u32 First;
    u32 Last;
    b32 IsConstant;
    ir_constant Constant;
} ir_reg_def;

internal u32
OptimizeRegIndex(u32 *RegOffsets, u32 FunctionIndex, u32 Reg)
{
    return(RegOffsets[FunctionIndex] + Reg);
}

internal u32
ResolveRegAlias(u32 *RegOffsets, u32 *Aliases, u32 FunctionIndex, u32 Reg)
{
    u32 Result = Reg;
    for(u32 Guard = 0; Guard < RegOffsets[FunctionIndex + 1] -
                             RegOffsets[FunctionIndex] + 1; ++Guard)
    {
        u32 Index = OptimizeRegIndex(RegOffsets, FunctionIndex, Result);
        u32 Next = Aliases[Index];
        if((Next == IRInvalid) || (Next == Result))
        {
            break;
        }
        Result = Next;
    }
    return(Result);
}

internal b32
GetConstantReg(u32 *RegOffsets, ir_reg_def *Defs, u32 InstructionIndex,
               u32 FunctionIndex, u32 Reg, ir_constant *Result)
{
    b32 Found = 0;
    if(Reg != IRInvalid)
    {
        u32 Index = OptimizeRegIndex(RegOffsets, FunctionIndex, Reg);
        ir_reg_def *Def = Defs + Index;
        if((Def->Count == 1) && Def->IsConstant &&
           (Def->First < InstructionIndex))
        {
            *Result = Def->Constant;
            Found = 1;
        }
    }
    return(Found);
}

internal b32
GetConstantOperand(u32 *RegOffsets, ir_reg_def *Defs, u32 InstructionIndex,
                   u32 FunctionIndex, ir_operand Operand,
                   ir_constant *Result)
{
    b32 Found = 0;
    if(Operand.Kind == IR_Const)
    {
        *Result = Operand.Const;
        Found = 1;
    }
    else if(Operand.Kind == IR_Reg)
    {
        Found = GetConstantReg(RegOffsets, Defs, InstructionIndex,
                               FunctionIndex, Operand.Reg, Result);
    }
    return(Found);
}

internal i32
CompareStrings(string Left, string Right)
{
    u32 Common = Minimum(Left.Count, Right.Count);
    i32 Result = 0;
    if(Common)
    {
        Result = memcmp(Left.Data, Right.Data, Common);
    }
    if(!Result)
    {
        if(Left.Count < Right.Count)
        {
            Result = -1;
        }
        else if(Left.Count > Right.Count)
        {
            Result = 1;
        }
    }
    return(Result);
}

internal b32
FoldBinary(ir_op Op, ir_constant Left, ir_constant Right,
           ir_constant *Result)
{
    b32 Folded = 0;
    if(Left.Type.Kind == Right.Type.Kind)
    {
        switch(Left.Type.Kind)
        {
            case Type_Number:
            {
                switch(Op)
                {
                    case IR_Add: { *Result = NumberConst(Left.Number + Right.Number); } break;
                    case IR_Sub: { *Result = NumberConst(Left.Number - Right.Number); } break;
                    case IR_Mul: { *Result = NumberConst(Left.Number * Right.Number); } break;
                    case IR_Div:
                    {
                        /* Leave 0/0 and x/0 alone so native IEEE behaviour
                         * decides the result. */
                        if(Right.Number != 0.0)
                        {
                            *Result = NumberConst(Left.Number / Right.Number);
                            Folded = 1;
                        }
                    } break;
                    case IR_Eq:  { *Result = BoolConst(Left.Number == Right.Number); } break;
                    case IR_Neq: { *Result = BoolConst(Left.Number != Right.Number); } break;
                    case IR_Lt:  { *Result = BoolConst(Left.Number < Right.Number); } break;
                    case IR_Le:  { *Result = BoolConst(Left.Number <= Right.Number); } break;
                    case IR_Gt:  { *Result = BoolConst(Left.Number > Right.Number); } break;
                    case IR_Ge:  { *Result = BoolConst(Left.Number >= Right.Number); } break;
                        InvalidDefaultCase;
                }
                if(Op != IR_Div)
                {
                    Folded = 1;
                }
            } break;

            case Type_String:
            {
                i32 Compare = CompareStrings(Left.String, Right.String);
                if((Op == IR_Eq) || (Op == IR_Neq))
                {
                    *Result = BoolConst((Op == IR_Eq) ? (Compare == 0) :
                                                        (Compare != 0));
                    Folded = 1;
                }
            } break;

            case Type_Bool:
            {
                u32 A = Left.Boolean ? 1 : 0;
                u32 B = Right.Boolean ? 1 : 0;
                switch(Op)
                {
                    case IR_Eq:  { *Result = BoolConst(A == B); } break;
                    case IR_Neq: { *Result = BoolConst(A != B); } break;
                    case IR_Lt:  { *Result = BoolConst(A < B); } break;
                    case IR_Le:  { *Result = BoolConst(A <= B); } break;
                    case IR_Gt:  { *Result = BoolConst(A > B); } break;
                    case IR_Ge:  { *Result = BoolConst(A >= B); } break;
                        InvalidDefaultCase;
                }
                Folded = 1;
            } break;

            case Type_Nil:
            {
                if((Op == IR_Eq) || (Op == IR_Neq))
                {
                    *Result = BoolConst(Op == IR_Eq);
                    Folded = 1;
                }
            } break;

            case Type_Unknown:
            case Type_Function:
            {
            } break;

            default:
            {
                InvalidCodePath;
            } break;
        }
    }
    return(Folded);
}

internal b32
FoldUnary(ir_op Op, ir_constant Operand, ir_constant *Result)
{
    b32 Folded = 0;
    if(Op == IR_Not)
    {
        if(Operand.Type.Kind == Type_Bool)
        {
            *Result = BoolConst(!Operand.Boolean);
            Folded = 1;
        }
    }
    else if(Op == IR_Neg)
    {
        if(Operand.Type.Kind == Type_Number)
        {
            *Result = NumberConst(-Operand.Number);
            Folded = 1;
        }
    }
    else if(Op == IR_Len)
    {
        if(Operand.Type.Kind == Type_String)
        {
            *Result = NumberConst((f64)Operand.String.Count);
            Folded = 1;
        }
    }
    return(Folded);
}

internal void
ReplaceWithConstant(ir_instruction *Instr, ir_constant Constant,
                    u32 *RegOffsets, ir_reg_def *Defs)
{
    Instr->Op = IR_LoadConst;
    Instr->A = IRConstOperand(Constant);
    Instr->B = IRNoOperand();
    Instr->Target = IRInvalid;
    Instr->ArgBase = 0;
    Instr->ArgCount = 0;
    Instr->CalleeIndex = IRInvalid;

    if(Instr->Dst.Kind == IR_Reg)
    {
        u32 Key = OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                   Instr->Dst.Reg);
        ir_reg_def *Def = Defs + Key;
        if(Def->Count == 1)
        {
            Def->IsConstant = 1;
            Def->Constant = Constant;
        }
    }
}

internal b32
IsPureDefinition(ir_op Op)
{
    b32 Result = 0;
    switch(Op)
    {
        case IR_LoadConst:
        case IR_LoadFunction:
        case IR_LoadSymbol:
        case IR_Move:
        case IR_Add:
        case IR_Sub:
        case IR_Mul:
        case IR_Div:
        case IR_Neg:
        case IR_Not:
        case IR_Len:
        case IR_Eq:
        case IR_Neq:
        case IR_Lt:
        case IR_Le:
        case IR_Gt:
        case IR_Ge:
        {
            Result = 1;
        } break;

        case IR_StoreSymbol:
        case IR_Jump:
        case IR_JumpIfFalse:
        case IR_JumpIfTrue:
        case IR_Call:
        case IR_Return:
        case IR_Count:
        {
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }
    return(Result);
}

internal void
PushWorkItem(u32 *Worklist, u32 *WorkCount, u32 Value)
{
    Worklist[(*WorkCount)++] = Value;
}

internal u32
SkipDeadInstructions(u32 Index, b32 *Dead, u32 InstructionCount)
{
    while((Index < InstructionCount) && Dead[Index])
    {
        ++Index;
    }
    return(Index);
}

internal u32
FollowJumpChain(ir_program *IR, b32 *Dead, u32 Target)
{
    u32 Result = SkipDeadInstructions(Target, Dead, IR->InstructionCount);
    for(u32 Guard = 0; Guard < IR->InstructionCount + 1; ++Guard)
    {
        if(Result >= IR->InstructionCount)
        {
            break;
        }
        ir_instruction *Instr = IR->Instructions + Result;
        if((Instr->Op != IR_Jump) || (Instr->Target == IRInvalid))
        {
            break;
        }
        u32 Next = SkipDeadInstructions(Instr->Target, Dead,
                                        IR->InstructionCount);
        if((Next == Result) || (Next > IR->InstructionCount))
        {
            break;
        }
        Result = Next;
    }
    return(Result);
}

internal void
CleanupJumps(ir_program *IR, b32 *Dead, b32 *FunctionEntry)
{
    u32 InstructionCount = IR->InstructionCount;
    for(u32 Index = 0; Index < InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if((Instr->Op == IR_Jump) || (Instr->Op == IR_JumpIfFalse) ||
           (Instr->Op == IR_JumpIfTrue))
        {
            if(Instr->Target != IRInvalid)
            {
                Instr->Target = FollowJumpChain(IR, Dead, Instr->Target);
            }
            if(Dead[Index] || FunctionEntry[Index] ||
               (Instr->Target != Index + 1))
            {
                continue;
            }
            if((Instr->Target < InstructionCount) &&
               FunctionEntry[Instr->Target])
            {
                continue;
            }
            Dead[Index] = 1;
        }
    }
}

internal void
OptimizeIRPass(ir_program *IR)
{
    u32 InstructionCount = IR->InstructionCount;
    if(!InstructionCount)
    {
        return;
    }

    /* All optimization tables are keyed by (function, virtual register). The
     * same register number is reused in every function frame. */
    u32 *RegOffsets = PushArray(IR->Arena, IR->FunctionCount + 1, u32);
    u32 TotalRegs = 0;
    for(u32 FunctionIndex = 0; FunctionIndex < IR->FunctionCount;
        ++FunctionIndex)
    {
        RegOffsets[FunctionIndex] = TotalRegs;
        TotalRegs += IR->Functions[FunctionIndex].RegCount;
    }
    RegOffsets[IR->FunctionCount] = TotalRegs;

    ir_reg_def *Defs = PushArray(IR->Arena, TotalRegs + 1, ir_reg_def);
    u32 *Aliases = PushArray(IR->Arena, TotalRegs + 1, u32);
    b32 *IsCallArg = PushArray(IR->Arena, TotalRegs + 1, b32);
    b32 *FunctionEntry = PushArray(IR->Arena, InstructionCount + 1, b32);
    b32 *Dead = PushArray(IR->Arena, InstructionCount + 1, b32);
    b32 *Reachable = PushArray(IR->Arena, InstructionCount + 1, b32);
    b32 *Used = PushArray(IR->Arena, TotalRegs + 1, b32);
    u32 *OldToNew = PushArray(IR->Arena, InstructionCount + 1, u32);
    u32 *Worklist = PushArray(IR->Arena, InstructionCount + 2, u32);
    ZeroSize((TotalRegs + 1) * sizeof(*Defs), Defs);
    ZeroSize((TotalRegs + 1) * sizeof(*Aliases), Aliases);
    ZeroSize((TotalRegs + 1) * sizeof(*IsCallArg), IsCallArg);
    ZeroSize((InstructionCount + 1) * sizeof(*FunctionEntry), FunctionEntry);
    ZeroSize((InstructionCount + 1) * sizeof(*Dead), Dead);
    ZeroSize((InstructionCount + 1) * sizeof(*Reachable), Reachable);
    ZeroSize((TotalRegs + 1) * sizeof(*Used), Used);
    for(u32 Index = 0; Index <= TotalRegs; ++Index)
    {
        Aliases[Index] = IRInvalid;
    }
    for(u32 FunctionIndex = 0; FunctionIndex < IR->FunctionCount;
        ++FunctionIndex)
    {
        u32 Entry = IR->Functions[FunctionIndex].Entry;
        if(Entry < InstructionCount)
        {
            FunctionEntry[Entry] = 1;
        }
    }

    for(u32 Index = 0; Index < InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if(Instr->Dst.Kind == IR_Reg)
        {
            u32 Key = OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                       Instr->Dst.Reg);
            ir_reg_def *Def = Defs + Key;
            if(Def->Count++ == 0)
            {
                Def->First = Index;
                Def->IsConstant = (Instr->Op == IR_LoadConst) &&
                                  (Instr->A.Kind == IR_Const);
                if(Def->IsConstant)
                {
                    Def->Constant = Instr->A.Const;
                }
            }
            else
            {
                Def->IsConstant = 0;
            }
            Def->Last = Index;
        }

        if(Instr->Op == IR_Call)
        {
            for(u32 ArgIndex = 0; ArgIndex < Instr->ArgCount; ++ArgIndex)
            {
                u32 Reg = Instr->ArgBase + ArgIndex;
                u32 Key = OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                           Reg);
                IsCallArg[Key] = 1;
            }
        }
    }

    /* A move can disappear when its destination is defined once, its source is
     * already stable, and neither register is a call argument slot. Call
     * arguments stay in fixed contiguous registers for the backend. */
    for(u32 Index = 0; Index < InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if((Instr->Op != IR_Move) || (Instr->Dst.Kind != IR_Reg) ||
           (Instr->A.Kind != IR_Reg))
        {
            continue;
        }

        u32 DstKey = OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                      Instr->Dst.Reg);
        u32 SrcKey = OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                      Instr->A.Reg);
        ir_reg_def *DstDef = Defs + DstKey;
        ir_reg_def *SrcDef = Defs + SrcKey;
        if((DstDef->Count != 1) || IsCallArg[DstKey] || !SrcDef->Count ||
           (SrcDef->Last >= Index))
        {
            continue;
        }

        u32 FinalReg = ResolveRegAlias(RegOffsets, Aliases,
                                       Instr->OwnerFunction, Instr->A.Reg);
        if(FinalReg != Instr->Dst.Reg)
        {
            Aliases[DstKey] = FinalReg;
        }
    }

    for(u32 Index = 0; Index < InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if(Instr->A.Kind == IR_Reg)
        {
            Instr->A.Reg = ResolveRegAlias(RegOffsets, Aliases,
                                           Instr->OwnerFunction,
                                           Instr->A.Reg);
        }
        if(Instr->B.Kind == IR_Reg)
        {
            Instr->B.Reg = ResolveRegAlias(RegOffsets, Aliases,
                                           Instr->OwnerFunction,
                                           Instr->B.Reg);
        }
    }

    /* Folding one instruction can expose another, so run to a small fixed
     * point. ReplaceWithConstant keeps the def table in sync. */
    for(u32 FoldPass = 0; FoldPass < 4; ++FoldPass)
    {
        b32 Changed = 0;
        for(u32 Index = 0; Index < InstructionCount; ++Index)
        {
            ir_instruction *Instr = IR->Instructions + Index;
            if(Dead[Index])
            {
                continue;
            }

            ir_constant Constant = {0};
            if((Instr->Op == IR_JumpIfFalse) || (Instr->Op == IR_JumpIfTrue))
            {
                if(GetConstantOperand(RegOffsets, Defs, Index,
                                      Instr->OwnerFunction, Instr->A,
                                      &Constant))
                {
                    b32 Value = Constant.Type.Kind == Type_Bool ?
                                (Constant.Boolean != 0) : 0;
                    b32 TakeBranch = (Instr->Op == IR_JumpIfFalse) ? !Value :
                                                                     Value;
                    if(TakeBranch)
                    {
                        Instr->Op = IR_Jump;
                        Instr->A = IRNoOperand();
                    }
                    else
                    {
                        Dead[Index] = 1;
                    }
                    Changed = 1;
                }
                continue;
            }

            if(IsPureDefinition(Instr->Op))
            {
                ir_constant Left = {0};
                ir_constant Right = {0};
                if((Instr->Op == IR_Add) || (Instr->Op == IR_Sub) ||
                   (Instr->Op == IR_Mul) || (Instr->Op == IR_Div) ||
                   (Instr->Op == IR_Eq) || (Instr->Op == IR_Neq) ||
                   (Instr->Op == IR_Lt) || (Instr->Op == IR_Le) ||
                   (Instr->Op == IR_Gt) || (Instr->Op == IR_Ge))
                {
                    if(GetConstantOperand(RegOffsets, Defs, Index,
                                          Instr->OwnerFunction, Instr->A,
                                          &Left) &&
                       GetConstantOperand(RegOffsets, Defs, Index,
                                          Instr->OwnerFunction, Instr->B,
                                          &Right) &&
                       FoldBinary(Instr->Op, Left, Right, &Constant))
                    {
                        ReplaceWithConstant(Instr, Constant, RegOffsets, Defs);
                        Changed = 1;
                    }
                }
                else if((Instr->Op == IR_Not) || (Instr->Op == IR_Neg) ||
                        (Instr->Op == IR_Len))
                {
                    if(GetConstantOperand(RegOffsets, Defs, Index,
                                          Instr->OwnerFunction, Instr->A,
                                          &Left) &&
                       FoldUnary(Instr->Op, Left, &Constant))
                    {
                        ReplaceWithConstant(Instr, Constant, RegOffsets, Defs);
                        Changed = 1;
                    }
                }
                else if((Instr->Op == IR_Move) && (Instr->A.Kind == IR_Const))
                {
                    ReplaceWithConstant(Instr, Instr->A.Const, RegOffsets, Defs);
                    Changed = 1;
                }
            }
        }
        if(!Changed)
        {
            break;
        }
    }

    CleanupJumps(IR, Dead, FunctionEntry);

    /* Chunks and function bodies are roots. Anything the control-flow graph
     * cannot reach is removed; this is what drops the unused side of a folded
     * `if` even when it contains calls. */
    u32 WorkCount = 0;
    if(!Dead[0] && !Reachable[0])
    {
        Reachable[0] = 1;
        PushWorkItem(Worklist, &WorkCount, 0);
    }
    for(u32 FunctionIndex = 0; FunctionIndex < IR->FunctionCount;
        ++FunctionIndex)
    {
        u32 Entry = IR->Functions[FunctionIndex].Entry;
        if((Entry < InstructionCount) && !Dead[Entry] && !Reachable[Entry])
        {
            Reachable[Entry] = 1;
            PushWorkItem(Worklist, &WorkCount, Entry);
        }
    }

    while(WorkCount)
    {
        u32 Index = Worklist[--WorkCount];
        ir_instruction *Instr = IR->Instructions + Index;

        if(Instr->Op == IR_Return)
        {
            continue;
        }

        if((Instr->Op == IR_Jump) || (Instr->Op == IR_JumpIfFalse) ||
           (Instr->Op == IR_JumpIfTrue))
        {
            u32 Target = SkipDeadInstructions(Instr->Target, Dead,
                                              InstructionCount);
            if((Target < InstructionCount) && !Reachable[Target])
            {
                Reachable[Target] = 1;
                PushWorkItem(Worklist, &WorkCount, Target);
            }
        }

        if(Instr->Op != IR_Jump)
        {
            u32 Next = SkipDeadInstructions(Index + 1, Dead, InstructionCount);
            if((Next < InstructionCount) && !Reachable[Next])
            {
                Reachable[Next] = 1;
                PushWorkItem(Worklist, &WorkCount, Next);
            }
        }
    }

    for(u32 Index = 0; Index < InstructionCount; ++Index)
    {
        if(!Reachable[Index] && !FunctionEntry[Index])
        {
            Dead[Index] = 1;
        }
    }
    CleanupJumps(IR, Dead, FunctionEntry);

    /* Remove pure definitions whose destination is never read. Repeat because
     * deleting one instruction can make its producer dead too. */
    for(u32 Pass = 0; Pass < 4; ++Pass)
    {
        ZeroSize((TotalRegs + 1) * sizeof(*Used), Used);
        for(u32 Index = 0; Index < InstructionCount; ++Index)
        {
            if(Dead[Index])
            {
                continue;
            }
            ir_instruction *Instr = IR->Instructions + Index;
            if(Instr->A.Kind == IR_Reg)
            {
                Used[OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                      Instr->A.Reg)] = 1;
            }
            if(Instr->B.Kind == IR_Reg)
            {
                Used[OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                      Instr->B.Reg)] = 1;
            }
            if(Instr->Op == IR_Call)
            {
                for(u32 ArgIndex = 0; ArgIndex < Instr->ArgCount; ++ArgIndex)
                {
                    u32 Reg = Instr->ArgBase + ArgIndex;
                    Used[OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                          Reg)] = 1;
                }
            }
        }

        b32 Removed = 0;
        for(u32 Index = 0; Index < InstructionCount; ++Index)
        {
            ir_instruction *Instr = IR->Instructions + Index;
            if(Dead[Index] || FunctionEntry[Index] ||
               (Instr->Dst.Kind != IR_Reg) || !IsPureDefinition(Instr->Op))
            {
                continue;
            }
            u32 Key = OptimizeRegIndex(RegOffsets, Instr->OwnerFunction,
                                       Instr->Dst.Reg);
            if(!Used[Key])
            {
                Dead[Index] = 1;
                Removed = 1;
            }
        }
        if(!Removed)
        {
            break;
        }
    }

    u32 NewCount = 0;
    for(u32 Index = 0; Index < InstructionCount; ++Index)
    {
        OldToNew[Index] = NewCount;
        if(!Dead[Index])
        {
            IR->Instructions[NewCount++] = IR->Instructions[Index];
        }
    }
    OldToNew[InstructionCount] = NewCount;

    for(u32 Index = 0; Index < NewCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if(Instr->Target != IRInvalid)
        {
            u32 Target = Instr->Target;
            if(Target <= InstructionCount)
            {
                Target = OldToNew[Target];
            }
            if(Target > NewCount)
            {
                Target = NewCount;
            }
            Instr->Target = Target;
        }
    }

    for(u32 FunctionIndex = 0; FunctionIndex < IR->FunctionCount;
        ++FunctionIndex)
    {
        ir_function_info *Info = IR->Functions + FunctionIndex;
        if(Info->Entry != IRInvalid)
        {
            u32 Entry = OldToNew[Info->Entry];
            if(Entry >= NewCount)
            {
                Entry = NewCount - 1;
            }
            Info->Entry = Entry;
        }
    }

    IR->InstructionCount = NewCount;
}

internal void
OptimizeIR(ir_program *IR)
{
    /* Compaction exposes new jump-to-next pairs and new constant producers.
     * Two cheap passes settle this IR; a third was measured as a no-op on the
     * benchmark corpus, so it is not part of the common path. */
    for(u32 Pass = 0; Pass < 2; ++Pass)
    {
        u32 Before = IR->InstructionCount;
        OptimizeIRPass(IR);
        if(IR->InstructionCount == Before)
        {
            break;
        }
    }
}

internal char *
IROpName(ir_op Op)
{
    char *Result = "unknown";
    switch(Op)
    {
        case IR_LoadConst:    { Result = "load_const"; } break;
        case IR_LoadFunction: { Result = "load_func"; } break;
        case IR_LoadSymbol:   { Result = "load_sym"; } break;
        case IR_StoreSymbol:  { Result = "store_sym"; } break;
        case IR_Move:         { Result = "move"; } break;
        case IR_Add:          { Result = "add"; } break;
        case IR_Sub:          { Result = "sub"; } break;
        case IR_Mul:          { Result = "mul"; } break;
        case IR_Div:          { Result = "div"; } break;
        case IR_Neg:          { Result = "neg"; } break;
        case IR_Not:          { Result = "not"; } break;
        case IR_Len:          { Result = "len"; } break;
        case IR_Eq:           { Result = "eq"; } break;
        case IR_Neq:          { Result = "neq"; } break;
        case IR_Lt:           { Result = "lt"; } break;
        case IR_Le:           { Result = "le"; } break;
        case IR_Gt:           { Result = "gt"; } break;
        case IR_Ge:           { Result = "ge"; } break;
        case IR_Jump:         { Result = "jmp"; } break;
        case IR_JumpIfFalse:  { Result = "jf"; } break;
        case IR_JumpIfTrue:   { Result = "jt"; } break;
        case IR_Call:         { Result = "call"; } break;
        case IR_Return:       { Result = "ret"; } break;
        case IR_Count:        { Result = "count"; } break;

            InvalidDefaultCase;
    }
    return(Result);
}

internal void
DumpOperand(ir_operand Operand)
{
    switch(Operand.Kind)
    {
        case IR_None:      { printf("-"); } break;
        case IR_Reg:       { printf("r%u", Operand.Reg); } break;
        case IR_Symbol:    { printf("sym%u", Operand.SymbolIndex); } break;
        case IR_Function:  { printf("fn%u", Operand.FunctionIndex); } break;
        case IR_Const:
            {
                switch(Operand.Const.Type.Kind)
                {
                    case Type_Nil:    { printf("nil"); } break;
                    case Type_Bool:   { printf("%s", Operand.Const.Boolean ? "true" : "false"); } break;
                    case Type_Number: { printf("%g", Operand.Const.Number); } break;
                    case Type_String: { printf("\"%.*s\"", (int)Operand.Const.String.Count, Operand.Const.String.Data); } break;
                        InvalidDefaultCase;
                }
            } break;

            InvalidDefaultCase;
    }
}

internal void
DumpIR(ir_program *IR)
{
    for(u32 Index = 0; Index < IR->InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        printf("%4u  %-11s ", Index, IROpName(Instr->Op));
        DumpOperand(Instr->Dst);
        printf("  ");
        DumpOperand(Instr->A);
        printf("  ");
        DumpOperand(Instr->B);
        if(Instr->Target != IRInvalid)
        {
            printf("  -> %u", Instr->Target);
        }
        if(Instr->Op == IR_Call)
        {
            printf("  fn%u args=r%u+%u", Instr->CalleeIndex, Instr->ArgBase,
                   Instr->ArgCount);
        }
        printf("\n");
    }
}
