/* ========================================================================

   $File: x86.c
   $Date: 2026/11/20
   $Revision: 1

   Textual AMD64 backend. The IR is already typed and flat, so this file does
   no analysis: it walks the instruction array and prints machine code. Every
   virtual register is a 16-byte frame slot. Numbers use the first 8 bytes as
   a double, booleans/integers/function pointers use the first 8 bytes as an
   integer, and strings use both words as a pointer/length pair.

   ======================================================================== */

#include "x86.h"

internal void Out(asm_emitter *E, char *Format, ...);

internal void
Out(asm_emitter *E, char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    vfprintf(E->File, Format, Args);
    va_end(Args);
}

internal u64
DoubleBits(f64 Number)
{
    union
    {
        f64 Number;
        u64 Bits;
    } Value;
    Value.Number = Number;
    return(Value.Bits);
}

internal char *
NewLabel(asm_emitter *E, char *Prefix)
{
    char *Result = PushArray(E->Arena, 32, char);
    snprintf(Result, 32, ".L%s%u", Prefix, E->Unique++);
    return(Result);
}

internal asm_string *
InternString(asm_emitter *E, string Value)
{
    asm_string *Result = 0;
    for(u32 Index = 0; Index < E->StringCount; ++Index)
    {
        if(StringsEqual(E->Strings[Index].Value, Value))
        {
            Result = E->Strings + Index;
            break;
        }
    }

    if(!Result)
    {
        Assert(E->StringCount < E->StringCapacity);
        Result = E->Strings + E->StringCount++;
        Result->Value = Value;
        Result->DataLabel = PushArray(E->Arena, 32, char);
        snprintf(Result->DataLabel, 32, ".LC%u", E->StringCount - 1);
    }

    return(Result);
}

internal char *
StringDataLabel(asm_emitter *E, string Value)
{
    return(InternString(E, Value)->DataLabel);
}

internal u32
StringLength(asm_emitter *E, string Value)
{
    return(InternString(E, Value)->Value.Count);
}

internal char *
InternNumber(asm_emitter *E, f64 Number)
{
    u64 Bits = DoubleBits(Number);
    char *Result = 0;

    for(u32 Index = 0; Index < E->NumberCount; ++Index)
    {
        if(E->Numbers[Index].Bits == Bits)
        {
            Result = E->Numbers[Index].Label;
            break;
        }
    }

    if(!Result)
    {
        Assert(E->NumberCount < E->NumberCapacity);
        Result = PushArray(E->Arena, 32, char);
        snprintf(Result, 32, ".LN%u", E->NumberCount);
        E->Numbers[E->NumberCount].Bits = Bits;
        E->Numbers[E->NumberCount].Label = Result;
        ++E->NumberCount;
    }

    return(Result);
}

internal u32
RegOffset(u32 Reg)
{
    return((Reg + 1) * 16);
}

internal u32
SlotOffset(ir_program *IR, u32 FunctionIndex, u32 Slot)
{
    /* Registers and local/parameter slots must not overlap: a function that
     * defines another function stores a code pointer in r0 while its own
     * parameter still lives in slot 0. */
    u32 RegCount = IR->Functions[FunctionIndex].RegCount;
    return((RegCount + Slot + 1) * 16);
}

internal u32
FrameSize(asm_emitter *E, u32 FunctionIndex)
{
    return(E->FrameSizes[FunctionIndex]);
}

internal void
SymbolAddress(asm_emitter *E, ir_program *IR, u32 SymbolIndex, char *Reg)
{
    semantic_symbol *Symbol = GetSymbol(IR->Semantic, SymbolIndex);
    if(Symbol->IsGlobal)
    {
        Out(E, "lea %s, [rip + .LG%u]\n", Reg, SymbolIndex);
    }
    else
    {
        Out(E, "lea %s, [rbp - %u]\n", Reg,
            SlotOffset(IR, Symbol->OwnerFunction, Symbol->Slot));
    }
}

internal void
StoreQword(asm_emitter *E, u32 RegOffsetValue, char *Source)
{
    Out(E, "mov [rbp - %u], %s\n", RegOffsetValue, Source);
}

internal void
ZeroRegUpper(asm_emitter *E, u32 RegOffsetValue)
{
    Out(E, "mov qword ptr [rbp - %u], 0\n", RegOffsetValue - 8);
}

internal void
CopyReg(asm_emitter *E, u32 DstReg, u32 SrcReg)
{
    u32 Dst = RegOffset(DstReg);
    u32 Src = RegOffset(SrcReg);
    Out(E, "mov rax, [rbp - %u]\n", Src);
    Out(E, "mov rdx, [rbp - %u]\n", Src - 8);
    Out(E, "mov [rbp - %u], rax\n", Dst);
    Out(E, "mov [rbp - %u], rdx\n", Dst - 8);
}

internal void
StoreNumber(asm_emitter *E, u32 RegOffsetValue, char *Xmm)
{
    Out(E, "movsd [rbp - %u], %s\n", RegOffsetValue, Xmm);
    ZeroRegUpper(E, RegOffsetValue);
}

internal void
StoreInteger(asm_emitter *E, u32 RegOffsetValue)
{
    Out(E, "mov [rbp - %u], rax\n", RegOffsetValue);
    ZeroRegUpper(E, RegOffsetValue);
}

internal void
EmitPrologue(asm_emitter *E, ir_program *IR, u32 FunctionIndex)
{
    u32 Frame = FrameSize(E, FunctionIndex);
    Out(E, "push rbp\n");
    Out(E, "mov rbp, rsp\n");
    if(Frame)
    {
        Out(E, "sub rsp, %u\n", Frame);
    }
    else
    {
        Out(E, "sub rsp, 0\n");
    }

    function_signature *Sig = GetFunction(IR->Semantic, FunctionIndex);
    if((FunctionIndex != ChunkFunctionIndex) && !Sig->IsBuiltin)
    {
        for(u32 Index = 0; Index < Sig->ParamCount; ++Index)
        {
            u32 SymbolIndex = Sig->FirstParamSymbol + Index;
            Assert(SymbolIndex < IR->Semantic->SymbolCount);
            semantic_symbol *Symbol = GetSymbol(IR->Semantic, SymbolIndex);
            u32 Offset = SlotOffset(IR, FunctionIndex, Symbol->Slot);

            Out(E, "mov rax, [rdi + %u]\n", Index * 16);
            Out(E, "mov [rbp - %u], rax\n", Offset);
            Out(E, "mov rax, [rdi + %u]\n", Index * 16 + 8);
            Out(E, "mov [rbp - %u], rax\n", Offset - 8);
        }
    }
}

internal void
EmitLoadConst(asm_emitter *E, ir_instruction *Instr, u32 Dst)
{
    ir_constant *Constant = &Instr->A.Const;
    switch(Constant->Type.Kind)
    {
        case Type_Nil:
        {
            Out(E, "mov qword ptr [rbp - %u], 0\n", Dst);
            ZeroRegUpper(E, Dst);
        } break;

        case Type_Bool:
        {
            Out(E, "mov rax, %u\n", (u32)(Constant->Boolean ? 1 : 0));
            StoreInteger(E, Dst);
        } break;

        case Type_Number:
        {
            char *Label = InternNumber(E, Constant->Number);
            Out(E, "mov rax, [rip + %s]\n", Label);
            StoreInteger(E, Dst);
        } break;

        case Type_String:
        {
            char *Label = StringDataLabel(E, Constant->String);
            Out(E, "lea rax, [rip + %s]\n", Label);
            StoreQword(E, Dst, "rax");
            Out(E, "mov qword ptr [rbp - %u], %u\n", Dst - 8,
                StringLength(E, Constant->String));
        } break;

        InvalidDefaultCase;
    }
}

internal void
EmitLoadSymbol(asm_emitter *E, ir_program *IR, ir_instruction *Instr, u32 Dst)
{
    SymbolAddress(E, IR, Instr->A.SymbolIndex, "rdx");
    Out(E, "mov rax, [rdx]\n");
    Out(E, "mov [rbp - %u], rax\n", Dst);
    Out(E, "mov rax, [rdx + 8]\n");
    Out(E, "mov [rbp - %u], rax\n", Dst - 8);
}

internal void
EmitStoreSymbol(asm_emitter *E, ir_program *IR, ir_instruction *Instr)
{
    u32 Src = RegOffset(Instr->B.Reg);
    SymbolAddress(E, IR, Instr->A.SymbolIndex, "rdx");
    Out(E, "mov rax, [rbp - %u]\n", Src);
    Out(E, "mov [rdx], rax\n");
    Out(E, "mov rax, [rbp - %u]\n", Src - 8);
    Out(E, "mov [rdx + 8], rax\n");
}

internal void
EmitArithmeticCore(asm_emitter *E, ir_instruction *Instr, u32 A, u32 B)
{
    Out(E, "movsd xmm0, [rbp - %u]\n", A);
    if(B)
    {
        Out(E, "movsd xmm1, [rbp - %u]\n", B);
    }

    switch(Instr->Op)
    {
        case IR_Add: { Out(E, "addsd xmm0, xmm1\n"); } break;
        case IR_Sub: { Out(E, "subsd xmm0, xmm1\n"); } break;
        case IR_Mul: { Out(E, "mulsd xmm0, xmm1\n"); } break;
        case IR_Div: { Out(E, "divsd xmm0, xmm1\n"); } break;
        case IR_Neg:
        {
            Out(E, "xorpd xmm0, xmm0\n");
            Out(E, "subsd xmm0, [rbp - %u]\n", A);
        } break;

        InvalidDefaultCase;
    }
}

internal void
EmitArithmetic(asm_emitter *E, ir_instruction *Instr, u32 Dst, u32 A, u32 B)
{
    EmitArithmeticCore(E, Instr, A, B);
    StoreNumber(E, Dst, "xmm0");
}

internal void
EmitCompareCore(asm_emitter *E, ir_program *IR, ir_instruction *Instr, u32 A,
                u32 B)
{
    lua_type OperandType =
        GetRegType(IR, Instr->OwnerFunction, Instr->A.Reg);
    char *SetTrue = 0;

    switch(Instr->Op)
    {
        case IR_Eq:    { SetTrue = "sete"; } break;
        case IR_Neq:   { SetTrue = "setne"; } break;
        case IR_Lt:    { SetTrue = "setb"; } break;
        case IR_Le:    { SetTrue = "setbe"; } break;
        case IR_Gt:    { SetTrue = "seta"; } break;
        case IR_Ge:    { SetTrue = "setae"; } break;

        InvalidDefaultCase;
    }

    if(OperandType.Kind == Type_String)
    {
        char *LengthMismatch = NewLabel(E, "NEQ");
        char *Done = NewLabel(E, "CMPDONE");
        Out(E, "mov rax, [rbp - %u]\n", A - 8);
        Out(E, "cmp rax, [rbp - %u]\n", B - 8);
        Out(E, "jne %s\n", LengthMismatch);
        Out(E, "mov rdi, [rbp - %u]\n", A);
        Out(E, "mov rsi, [rbp - %u]\n", B);
        Out(E, "mov rdx, [rbp - %u]\n", A - 8);
        Out(E, "call memcmp\n");
        Out(E, "test eax, eax\n");
        Out(E, "%s al\n", (Instr->Op == IR_Eq) ? "sete" : "setne");
        Out(E, "jmp %s\n", Done);
        Out(E, "%s:\n", LengthMismatch);
        Out(E, "mov eax, %u\n", (Instr->Op == IR_Eq) ? 0 : 1);
        Out(E, "%s:\n", Done);
        return;
    }

    if((OperandType.Kind == Type_Number) &&
       ((Instr->Op == IR_Lt) || (Instr->Op == IR_Le) ||
        (Instr->Op == IR_Gt) || (Instr->Op == IR_Ge)))
    {
        Out(E, "movsd xmm0, [rbp - %u]\n", A);
        Out(E, "movsd xmm1, [rbp - %u]\n", B);
        Out(E, "ucomisd xmm0, xmm1\n");
        switch(Instr->Op)
        {
            case IR_Lt: { SetTrue = "setb"; } break;
            case IR_Le: { SetTrue = "setbe"; } break;
            case IR_Gt: { SetTrue = "seta"; } break;
            case IR_Ge: { SetTrue = "setae"; } break;
            InvalidDefaultCase;
        }
        Out(E, "%s al\n", SetTrue);
        Out(E, "setnp dl\n");
        Out(E, "and al, dl\n");
        return;
    }

    if(OperandType.Kind == Type_Number)
    {
        Out(E, "movsd xmm0, [rbp - %u]\n", A);
        Out(E, "movsd xmm1, [rbp - %u]\n", B);
        Out(E, "ucomisd xmm0, xmm1\n");
        if(Instr->Op == IR_Eq)
        {
            Out(E, "sete al\n");
            Out(E, "setnp dl\n");
            Out(E, "and al, dl\n");
        }
        else
        {
            Out(E, "setne al\n");
            Out(E, "setp dl\n");
            Out(E, "or al, dl\n");
        }
        return;
    }

    Out(E, "mov rax, [rbp - %u]\n", A);
    Out(E, "cmp rax, [rbp - %u]\n", B);
    Out(E, "%s al\n", SetTrue);
}

internal void
EmitCompare(asm_emitter *E, ir_program *IR, ir_instruction *Instr, u32 Dst,
            u32 A, u32 B)
{
    EmitCompareCore(E, IR, Instr, A, B);
    Out(E, "movzx rax, al\n");
    StoreInteger(E, Dst);
}

internal void
EmitStoreReturn(asm_emitter *E, u32 Dst, lua_type Type)
{
    if(Type.Kind == Type_Number)
    {
        Out(E, "movsd [rbp - %u], xmm0\n", Dst);
        ZeroRegUpper(E, Dst);
    }
    else if(Type.Kind == Type_String)
    {
        Out(E, "mov [rbp - %u], rax\n", Dst);
        Out(E, "mov [rbp - %u], rdx\n", Dst - 8);
    }
    else
    {
        StoreInteger(E, Dst);
    }
}

internal void
EmitPrintBuiltin(asm_emitter *E, ir_program *IR, ir_instruction *Instr)
{
    for(u32 Index = 0; Index < Instr->ArgCount; ++Index)
    {
        u32 Reg = RegOffset(Instr->ArgBase + Index);
        lua_type Type = GetRegType(IR, Instr->OwnerFunction,
                                   Instr->ArgBase + Index);

        if(Index > 0)
        {
            Out(E, "lea rdi, [rip + .LFSP]\n");
            Out(E, "xor eax, eax\n");
            Out(E, "call printf\n");
        }

        switch(Type.Kind)
        {
            case Type_Number:
            {
                Out(E, "lea rdi, [rip + .LFNUM]\n");
                Out(E, "movsd xmm0, [rbp - %u]\n", Reg);
                Out(E, "mov eax, 1\n");
                Out(E, "call printf\n");
            } break;

            case Type_Bool:
            {
                Out(E, "cmp qword ptr [rbp - %u], 0\n", Reg);
                Out(E, "lea rdi, [rip + .LFCS]\n");
                Out(E, "lea rsi, [rip + %s]\n",
                    StringDataLabel(E, StringFromC("false")));
                Out(E, "lea rdx, [rip + %s]\n",
                    StringDataLabel(E, StringFromC("true")));
                Out(E, "cmovne rsi, rdx\n");
                Out(E, "xor eax, eax\n");
                Out(E, "call printf\n");
            } break;

            case Type_String:
            {
                /* printf("%.*s", precision, string): the int precision is the
                 * first variadic argument (esi), the pointer is the second. */
                Out(E, "lea rdi, [rip + .LFSTR]\n");
                Out(E, "mov esi, [rbp - %u]\n", Reg - 8);
                Out(E, "mov rdx, [rbp - %u]\n", Reg);
                Out(E, "xor eax, eax\n");
                Out(E, "call printf\n");
            } break;

            case Type_Nil:
            case Type_Function:
            default:
            {
                char *Text = (Type.Kind == Type_Function) ? "function" : "nil";
                Out(E, "lea rdi, [rip + .LFCS]\n");
                Out(E, "lea rsi, [rip + %s]\n",
                    StringDataLabel(E, StringFromC(Text)));
                Out(E, "xor eax, eax\n");
                Out(E, "call printf\n");
            } break;
        }
    }

    Out(E, "lea rdi, [rip + .LFNL]\n");
    Out(E, "xor eax, eax\n");
    Out(E, "call printf\n");
}

internal void
EmitTypeBuiltin(asm_emitter *E, ir_program *IR, ir_instruction *Instr,
                u32 Dst)
{
    lua_type Type = SimpleType(Type_Unknown);
    if(Instr->ArgCount > 0)
    {
        Type = GetRegType(IR, Instr->OwnerFunction, Instr->ArgBase);
    }

    char *Name = TypeName(Type);
    string Value = StringFromC(Name);
    Out(E, "lea rax, [rip + %s]\n", StringDataLabel(E, Value));
    Out(E, "mov [rbp - %u], rax\n", Dst);
    Out(E, "mov qword ptr [rbp - %u], %u\n", Dst - 8,
        StringLength(E, Value));
}

internal void
EmitNativeCall(asm_emitter *E, ir_program *IR, ir_instruction *Instr, u32 Dst)
{
    function_signature *Sig = GetFunction(IR->Semantic, Instr->CalleeIndex);
    if(Sig->IsBuiltin)
    {
        if(Sig->Builtin == Builtin_Print)
        {
            EmitPrintBuiltin(E, IR, Instr);
        }
        else
        {
            EmitTypeBuiltin(E, IR, Instr, Dst);
        }
        return;
    }

    u32 ArgBytes = Instr->ArgCount * 16;
    if(ArgBytes)
    {
        Out(E, "sub rsp, %u\n", ArgBytes);
    }

    for(u32 Index = 0; Index < Instr->ArgCount; ++Index)
    {
        u32 Reg = RegOffset(Instr->ArgBase + Index);
        Out(E, "mov rax, [rbp - %u]\n", Reg);
        Out(E, "mov [rsp + %u], rax\n", Index * 16);
        Out(E, "mov rax, [rbp - %u]\n", Reg - 8);
        Out(E, "mov [rsp + %u], rax\n", Index * 16 + 8);
    }

    Out(E, "lea rdi, [rsp]\n");
    Out(E, "call .LF%u\n", Instr->CalleeIndex);
    if(ArgBytes)
    {
        Out(E, "add rsp, %u\n", ArgBytes);
    }

    if(Dst)
    {
        EmitStoreReturn(E, Dst, Instr->Type);
    }
}

internal void
EmitReturn(asm_emitter *E, ir_program *IR, ir_instruction *Instr)
{
    (void)IR;
    if((Instr->A.Kind == IR_Reg) && (Instr->Type.Kind == Type_Number))
    {
        Out(E, "movsd xmm0, [rbp - %u]\n", RegOffset(Instr->A.Reg));
    }
    else if((Instr->A.Kind == IR_Reg) && (Instr->Type.Kind == Type_String))
    {
        u32 Reg = RegOffset(Instr->A.Reg);
        Out(E, "mov rax, [rbp - %u]\n", Reg);
        Out(E, "mov rdx, [rbp - %u]\n", Reg - 8);
    }
    else if(Instr->A.Kind == IR_Reg)
    {
        Out(E, "mov rax, [rbp - %u]\n", RegOffset(Instr->A.Reg));
    }
    else
    {
        Out(E, "xor eax, eax\n");
        Out(E, "xor edx, edx\n");
    }

    u32 Frame = FrameSize(E, Instr->OwnerFunction);
    if(Frame)
    {
        Out(E, "add rsp, %u\n", Frame);
    }
    Out(E, "pop rbp\n");
    Out(E, "ret\n");
}

internal b32
IsFunctionEntry(asm_emitter *E, u32 Index)
{
    return((Index < E->InstructionCount) ? E->FunctionEntries[Index] : 0);
}

internal b32
IsJumpTarget(asm_emitter *E, u32 Index)
{
    return((Index < E->InstructionCount) ? E->JumpTargets[Index] : 0);
}

internal b32
IsPackableArithmetic(ir_instruction *Instr)
{
    b32 Result = ((Instr->Op == IR_Add) || (Instr->Op == IR_Sub) ||
                  (Instr->Op == IR_Mul) || (Instr->Op == IR_Div)) &&
                 (Instr->Type.Kind == Type_Number) &&
                 (Instr->Dst.Kind == IR_Reg) && (Instr->A.Kind == IR_Reg) &&
                 (Instr->B.Kind == IR_Reg);
    return(Result);
}

internal b32
CanPackArithmeticPair(asm_emitter *E, ir_program *IR, u32 Index)
{
    b32 Result = 0;
    if((Index + 1) < IR->InstructionCount)
    {
        ir_instruction *First = IR->Instructions + Index;
        ir_instruction *Second = IR->Instructions + Index + 1;
        if(IsPackableArithmetic(First) && IsPackableArithmetic(Second) &&
           (First->OwnerFunction == Second->OwnerFunction) &&
           (First->Op == Second->Op) &&
           !IsFunctionEntry(E, Index) && !IsFunctionEntry(E, Index + 1) &&
           (First->Dst.Reg != Second->Dst.Reg) &&
           (First->Dst.Reg != Second->A.Reg) &&
           (First->Dst.Reg != Second->B.Reg))
        {
            Result = 1;
        }
    }
    return(Result);
}

internal void
EmitPackedArithmeticPair(asm_emitter *E, ir_program *IR, u32 Index)
{
    ir_instruction *First = IR->Instructions + Index;
    ir_instruction *Second = IR->Instructions + Index + 1;

    u32 Dst0 = RegOffset(First->Dst.Reg);
    u32 A0 = RegOffset(First->A.Reg);
    u32 B0 = RegOffset(First->B.Reg);
    u32 Dst1 = RegOffset(Second->Dst.Reg);
    u32 A1 = RegOffset(Second->A.Reg);
    u32 B1 = RegOffset(Second->B.Reg);

    Out(E, ".IR_%u:\n", Index);

    /* Two scalar value slots become the two lanes of one XMM register. The
     * upper word of each slot is a type tag and is zeroed separately below. */
    Out(E, "movsd xmm0, qword ptr [rbp - %u]\n", A0);
    Out(E, "movhpd xmm0, qword ptr [rbp - %u]\n", A1);
    Out(E, "movsd xmm1, qword ptr [rbp - %u]\n", B0);
    Out(E, "movhpd xmm1, qword ptr [rbp - %u]\n", B1);

    switch(First->Op)
    {
        case IR_Add: { Out(E, "addpd xmm0, xmm1\n"); } break;
        case IR_Sub: { Out(E, "subpd xmm0, xmm1\n"); } break;
        case IR_Mul: { Out(E, "mulpd xmm0, xmm1\n"); } break;
        case IR_Div: { Out(E, "divpd xmm0, xmm1\n"); } break;
        InvalidDefaultCase;
    }

    Out(E, "movlpd qword ptr [rbp - %u], xmm0\n", Dst0);
    Out(E, "movhpd qword ptr [rbp - %u], xmm0\n", Dst1);
    ZeroRegUpper(E, Dst0);
    ZeroRegUpper(E, Dst1);

    Out(E, ".IR_%u:\n", Index + 1);
}

internal b32
CanEmitArithmeticChainPair(asm_emitter *E, ir_program *IR, u32 Index)
{
    b32 Result = 0;
    if((Index + 1) < IR->InstructionCount)
    {
        ir_instruction *First = IR->Instructions + Index;
        ir_instruction *Second = IR->Instructions + Index + 1;
        b32 UsesFirstResult =
            ((Second->A.Kind == IR_Reg) &&
             (Second->A.Reg == First->Dst.Reg)) ||
            ((Second->B.Kind == IR_Reg) &&
             (Second->B.Reg == First->Dst.Reg));

        if(IsPackableArithmetic(First) && IsPackableArithmetic(Second) &&
           (First->OwnerFunction == Second->OwnerFunction) &&
           (First->Dst.Reg != Second->Dst.Reg) && UsesFirstResult &&
           !IsFunctionEntry(E, Index) &&
           !IsFunctionEntry(E, Index + 1) &&
           !IsJumpTarget(E, Index + 1))
        {
            Result = 1;
        }
    }
    return(Result);
}

internal void
EmitArithmeticChainPair(asm_emitter *E, ir_program *IR, u32 Index)
{
    ir_instruction *First = IR->Instructions + Index;
    ir_instruction *Second = IR->Instructions + Index + 1;

    u32 Dst0 = RegOffset(First->Dst.Reg);
    u32 A0 = RegOffset(First->A.Reg);
    u32 B0 = RegOffset(First->B.Reg);
    u32 Dst1 = RegOffset(Second->Dst.Reg);
    u32 A1 = (Second->A.Kind == IR_Reg) ? RegOffset(Second->A.Reg) : 0;
    u32 B1 = (Second->B.Kind == IR_Reg) ? RegOffset(Second->B.Reg) : 0;

    Out(E, ".IR_%u:\n", Index);

    /* Keep the first result live in xmm0 for the dependent second operation.
     * It is still spilled to its own slot because later code may read it. */
    EmitArithmeticCore(E, First, A0, B0);
    StoreNumber(E, Dst0, "xmm0");

    b32 AIsChain = (Second->A.Reg == First->Dst.Reg);
    b32 BIsChain = (Second->B.Reg == First->Dst.Reg);
    if(AIsChain && BIsChain)
    {
        Out(E, "movapd xmm1, xmm0\n");
    }
    else if(AIsChain)
    {
        Out(E, "movsd xmm1, qword ptr [rbp - %u]\n", B1);
    }
    else
    {
        Out(E, "movapd xmm1, xmm0\n");
        Out(E, "movsd xmm0, qword ptr [rbp - %u]\n", A1);
    }

    switch(Second->Op)
    {
        case IR_Add: { Out(E, "addsd xmm0, xmm1\n"); } break;
        case IR_Sub: { Out(E, "subsd xmm0, xmm1\n"); } break;
        case IR_Mul: { Out(E, "mulsd xmm0, xmm1\n"); } break;
        case IR_Div: { Out(E, "divsd xmm0, xmm1\n"); } break;
        InvalidDefaultCase;
    }

    StoreNumber(E, Dst1, "xmm0");
    Out(E, ".IR_%u:\n", Index + 1);
}

internal void
EmitJump(asm_emitter *E, ir_program *IR, char *Opcode, u32 Target)
{
    /* A function entry instruction has both the flat .IR label and the real
     * function label. Branches must use the function label: landing on the
     * .IR label would fall through the prologue again on every loop back-edge. */
    for(u32 FunctionIndex = 0; FunctionIndex < IR->FunctionCount;
        ++FunctionIndex)
    {
        if(IR->Functions[FunctionIndex].Entry == Target)
        {
            /* .FB is just past the prologue. .LF remains the callable entry. */
            Out(E, "%s .FB%u\n", Opcode, FunctionIndex);
            return;
        }
    }

    Out(E, "%s .IR_%u\n", Opcode, Target);
}

internal b32
IsCompareOp(ir_op Op)
{
    b32 Result = ((Op == IR_Eq) || (Op == IR_Neq) || (Op == IR_Lt) ||
                  (Op == IR_Le) || (Op == IR_Gt) || (Op == IR_Ge));
    return(Result);
}

internal b32
FusedCompareResultHasOneUse(ir_program *IR, u32 CompareIndex, u32 OwnerFunction,
                            u32 Reg)
{
    u32 Uses = 0;
    for(u32 Index = 0; Index < IR->InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if((Instr->OwnerFunction != OwnerFunction) || (Index == CompareIndex))
        {
            continue;
        }

        b32 IsFusedUse =
            ((Index == CompareIndex + 1) &&
             ((Instr->Op == IR_JumpIfFalse) || (Instr->Op == IR_JumpIfTrue)) &&
             (Instr->A.Kind == IR_Reg) && (Instr->A.Reg == Reg));
        if(IsFusedUse)
        {
            ++Uses;
            continue;
        }

        if(((Instr->A.Kind == IR_Reg) && (Instr->A.Reg == Reg)) ||
           ((Instr->B.Kind == IR_Reg) && (Instr->B.Reg == Reg)))
        {
            ++Uses;
        }
        if(Instr->Op == IR_Call)
        {
            for(u32 ArgIndex = 0; ArgIndex < Instr->ArgCount; ++ArgIndex)
            {
                if((Instr->ArgBase + ArgIndex) == Reg)
                {
                    ++Uses;
                }
            }
        }
    }
    return(Uses == 1);
}

internal b32
CanFuseCompareJump(asm_emitter *E, ir_program *IR, u32 Index)
{
    b32 Result = 0;
    if((Index + 1) < IR->InstructionCount)
    {
        ir_instruction *Compare = IR->Instructions + Index;
        ir_instruction *Jump = IR->Instructions + Index + 1;
        b32 JumpUsesCompare =
            ((Jump->Op == IR_JumpIfFalse) || (Jump->Op == IR_JumpIfTrue)) &&
            (Jump->A.Kind == IR_Reg) &&
            (Jump->A.Reg == Compare->Dst.Reg);

        if(IsCompareOp(Compare->Op) && (Compare->Dst.Kind == IR_Reg) &&
           (Compare->OwnerFunction == Jump->OwnerFunction) && JumpUsesCompare &&
           !IsFunctionEntry(E, Index) && !IsFunctionEntry(E, Index + 1) &&
           !IsJumpTarget(E, Index + 1) &&
           FusedCompareResultHasOneUse(IR, Index, Compare->OwnerFunction,
                                       Compare->Dst.Reg))
        {
            Result = 1;
        }
    }
    return(Result);
}

internal void
EmitFusedCompareJump(asm_emitter *E, ir_program *IR, u32 Index)
{
    ir_instruction *Compare = IR->Instructions + Index;
    ir_instruction *Jump = IR->Instructions + Index + 1;
    u32 A = (Compare->A.Kind == IR_Reg) ? RegOffset(Compare->A.Reg) : 0;
    u32 B = (Compare->B.Kind == IR_Reg) ? RegOffset(Compare->B.Reg) : 0;

    Out(E, ".IR_%u:\n", Index);
    EmitCompareCore(E, IR, Compare, A, B);
    Out(E, "test al, al\n");
    EmitJump(E, IR, (Jump->Op == IR_JumpIfTrue) ? "jnz" : "jz",
             Jump->Target);
    Out(E, ".IR_%u:\n", Index + 1);
}

internal b32
GetIntegerConstantBefore(ir_program *IR, u32 OwnerFunction, u32 Reg,
                         u32 BeforeIndex, i64 *Result)
{
    b32 Found = 0;
    for(u32 Index = BeforeIndex; Index > 0; --Index)
    {
        ir_instruction *Instr = IR->Instructions + (Index - 1);
        if((Instr->OwnerFunction != OwnerFunction) ||
           (Instr->Dst.Kind != IR_Reg) || (Instr->Dst.Reg != Reg))
        {
            continue;
        }

        if((Instr->Op == IR_LoadConst) && (Instr->A.Kind == IR_Const) &&
           (Instr->A.Const.Type.Kind == Type_Number))
        {
            f64 Number = Instr->A.Const.Number;
            if((Number >= -9007199254740992.0) &&
               (Number <= 9007199254740992.0) &&
               (Number == (f64)(i64)Number))
            {
                *Result = (i64)Number;
                Found = 1;
            }
        }
        break;
    }
    return(Found);
}

internal b32
IntegerLoopCounterHasNoOutsideUses(ir_program *IR, u32 OwnerFunction,
                                   u32 Index, u32 CounterReg)
{
    for(u32 InstrIndex = 0; InstrIndex < IR->InstructionCount; ++InstrIndex)
    {
        if((InstrIndex >= Index) && (InstrIndex <= Index + 3))
        {
            continue;
        }

        ir_instruction *Instr = IR->Instructions + InstrIndex;
        if(Instr->OwnerFunction != OwnerFunction)
        {
            continue;
        }

        if(((Instr->A.Kind == IR_Reg) && (Instr->A.Reg == CounterReg)) ||
           ((Instr->B.Kind == IR_Reg) && (Instr->B.Reg == CounterReg)))
        {
            return(0);
        }
        if(Instr->Op == IR_Call)
        {
            for(u32 ArgIndex = 0; ArgIndex < Instr->ArgCount; ++ArgIndex)
            {
                if((Instr->ArgBase + ArgIndex) == CounterReg)
                {
                    return(0);
                }
            }
        }
    }
    return(1);
}

internal b32
CanEmitIntegerEmptyLoop(asm_emitter *E, ir_program *IR, u32 Index)
{
    b32 Result = 0;
    if((Index + 3) >= IR->InstructionCount)
    {
        return(Result);
    }

    ir_instruction *Compare = IR->Instructions + Index;
    ir_instruction *Jump = IR->Instructions + Index + 1;
    ir_instruction *Add = IR->Instructions + Index + 2;
    ir_instruction *Back = IR->Instructions + Index + 3;
    b32 Shape =
        ((Compare->Op == IR_Gt) || (Compare->Op == IR_Lt)) &&
        (Compare->Dst.Kind == IR_Reg) &&
        (Compare->A.Kind == IR_Reg) && (Compare->B.Kind == IR_Reg) &&
        (Jump->Op == IR_JumpIfTrue) && (Jump->A.Kind == IR_Reg) &&
        (Jump->A.Reg == Compare->Dst.Reg) &&
        (Add->Op == IR_Add) && (Add->Dst.Kind == IR_Reg) &&
        (Add->Dst.Reg == Compare->A.Reg) &&
        (Add->A.Kind == IR_Reg) && (Add->A.Reg == Compare->A.Reg) &&
        (Add->B.Kind == IR_Reg) && (Back->Op == IR_Jump) &&
        (Back->Target == Index);

    if(!Shape || (Compare->OwnerFunction != Jump->OwnerFunction) ||
       (Compare->OwnerFunction != Add->OwnerFunction) ||
       (Compare->OwnerFunction != Back->OwnerFunction))
    {
        return(Result);
    }

    /* Index is the loop header and is expected to be a jump target. Incoming
     * branches must not land in the middle of the fused pattern. */
    if(IsFunctionEntry(E, Index))
    {
        return(Result);
    }
    for(u32 Offset = 1; Offset < 4; ++Offset)
    {
        if(IsFunctionEntry(E, Index + Offset) ||
           IsJumpTarget(E, Index + Offset))
        {
            return(Result);
        }
    }

    u32 CounterReg = Compare->A.Reg;
    if(!FusedCompareResultHasOneUse(IR, Index, Compare->OwnerFunction,
                                    Compare->Dst.Reg) ||
       !IntegerLoopCounterHasNoOutsideUses(IR, Compare->OwnerFunction, Index,
                                           CounterReg))
    {
        return(Result);
    }

    i64 Start = 0;
    i64 End = 0;
    i64 Step = 0;
    if(GetIntegerConstantBefore(IR, Compare->OwnerFunction, CounterReg,
                                Index, &Start) &&
       GetIntegerConstantBefore(IR, Compare->OwnerFunction, Compare->B.Reg,
                                Index, &End) &&
       GetIntegerConstantBefore(IR, Compare->OwnerFunction, Add->B.Reg,
                                Index, &Step) &&
       (Step != 0))
    {
        Result = ((Step > 0) && (Compare->Op == IR_Gt)) ||
                 ((Step < 0) && (Compare->Op == IR_Lt));
    }
    return(Result);
}

internal void
EmitIntegerEmptyLoop(asm_emitter *E, ir_program *IR, u32 Index)
{
    ir_instruction *Compare = IR->Instructions + Index;
    ir_instruction *Jump = IR->Instructions + Index + 1;
    u32 CounterReg = Compare->A.Reg;
    u32 EndReg = Compare->B.Reg;
    i64 Start = 0;
    i64 End = 0;
    i64 Step = 0;
    GetIntegerConstantBefore(IR, Compare->OwnerFunction, CounterReg, Index,
                             &Start);
    GetIntegerConstantBefore(IR, Compare->OwnerFunction, EndReg, Index, &End);
    GetIntegerConstantBefore(IR, Compare->OwnerFunction,
                             IR->Instructions[Index + 2].B.Reg, Index, &Step);

    char *LoopLabel = NewLabel(E, "IL");
    Out(E, ".IR_%u:\n", Index);
    Out(E, "mov r10, %lld\n", (long long)Start);
    Out(E, "mov r11, %lld\n", (long long)End);
    Out(E, "%s:\n", LoopLabel);
    Out(E, "cmp r10, r11\n");
    EmitJump(E, IR, (Step > 0) ? "jg" : "jl", Jump->Target);
    Out(E, ".IR_%u:\n", Index + 1);
    Out(E, ".IR_%u:\n", Index + 2);
    Out(E, "mov rax, %lld\n", (long long)Step);
    Out(E, "add r10, rax\n");
    Out(E, ".IR_%u:\n", Index + 3);
    Out(E, "jmp %s\n", LoopLabel);
}

internal void
EmitInstruction(asm_emitter *E, ir_program *IR, u32 Index)
{
    ir_instruction *Instr = IR->Instructions + Index;
    u32 Dst = (Instr->Dst.Kind == IR_Reg) ? RegOffset(Instr->Dst.Reg) : 0;
    u32 A = (Instr->A.Kind == IR_Reg) ? RegOffset(Instr->A.Reg) : 0;
    u32 B = (Instr->B.Kind == IR_Reg) ? RegOffset(Instr->B.Reg) : 0;

    Out(E, ".IR_%u:\n", Index);

    ir_function_info *Info = IR->Functions + Instr->OwnerFunction;
    if(Index == Info->Entry)
    {
        if(Instr->OwnerFunction == ChunkFunctionIndex)
        {
            Out(E, ".globl lua_main\n");
            Out(E, ".type lua_main, @function\n");
            Out(E, "lua_main:\n");
        }
        else
        {
            Out(E, ".LF%u:\n", Instr->OwnerFunction);
        }
        EmitPrologue(E, IR, Instr->OwnerFunction);
        Out(E, ".FB%u:\n", Instr->OwnerFunction);
    }

    switch(Instr->Op)
    {
        case IR_LoadConst:
        {
            EmitLoadConst(E, Instr, Dst);
        } break;

        case IR_LoadFunction:
        {
            Out(E, "lea rax, [rip + .LF%u]\n", Instr->A.FunctionIndex);
            Out(E, "mov [rbp - %u], rax\n", Dst);
            ZeroRegUpper(E, Dst);
        } break;

        case IR_LoadSymbol:
        {
            EmitLoadSymbol(E, IR, Instr, Dst);
        } break;

        case IR_StoreSymbol:
        {
            EmitStoreSymbol(E, IR, Instr);
        } break;

        case IR_Move:
        {
            CopyReg(E, Instr->Dst.Reg, Instr->A.Reg);
        } break;

        case IR_Add:
        case IR_Sub:
        case IR_Mul:
        case IR_Div:
        case IR_Neg:
        {
            EmitArithmetic(E, Instr, Dst, A, B);
        } break;

        case IR_Not:
        {
            Out(E, "cmp qword ptr [rbp - %u], 0\n", A);
            Out(E, "sete al\n");
            Out(E, "movzx rax, al\n");
            StoreInteger(E, Dst);
        } break;

        case IR_Len:
        {
            Out(E, "mov rax, [rbp - %u]\n", A - 8);
            Out(E, "cvtsi2sd xmm0, rax\n");
            StoreNumber(E, Dst, "xmm0");
        } break;

        case IR_Eq:
        case IR_Neq:
        case IR_Lt:
        case IR_Le:
        case IR_Gt:
        case IR_Ge:
        {
            EmitCompare(E, IR, Instr, Dst, A, B);
        } break;

        case IR_Jump:
        {
            EmitJump(E, IR, "jmp", Instr->Target);
        } break;

        case IR_JumpIfFalse:
        {
            Out(E, "cmp qword ptr [rbp - %u], 0\n", A);
            EmitJump(E, IR, "jz", Instr->Target);
        } break;

        case IR_JumpIfTrue:
        {
            Out(E, "cmp qword ptr [rbp - %u], 0\n", A);
            EmitJump(E, IR, "jnz", Instr->Target);
        } break;

        case IR_Call:
        {
            EmitNativeCall(E, IR, Instr, Dst);
        } break;

        case IR_Return:
        {
            EmitReturn(E, IR, Instr);
        } break;

        case IR_Count:
        {
            InvalidCodePath;
        } break;

        InvalidDefaultCase;
    }
}

internal void
EmitAscii(asm_emitter *E, string Value)
{
    for(u32 Index = 0; Index < Value.Count; ++Index)
    {
        u8 Byte = (u8)Value.Data[Index];
        if((Byte == '"') || (Byte == '\\'))
        {
            Out(E, "\\%c", (char)Byte);
        }
        else if((Byte >= 32) && (Byte < 127))
        {
            Out(E, "%c", (char)Byte);
        }
        else
        {
            Out(E, "\\%03o", Byte);
        }
    }
}

internal void
EmitData(asm_emitter *E, ir_program *IR)
{
    Out(E, ".section .rodata\n");
    Out(E, ".LFNUM:\n.ascii \"%%.14g\"\n.byte 0\n");
    Out(E, ".LFSTR:\n.ascii \"%%.*s\"\n.byte 0\n");
    Out(E, ".LFCS:\n.ascii \"%%s\"\n.byte 0\n");
    Out(E, ".LFSP:\n.ascii \" \"\n.byte 0\n");
    Out(E, ".LFNL:\n.ascii \"\\n\"\n.byte 0\n");

    for(u32 Index = 0; Index < E->StringCount; ++Index)
    {
        asm_string *Entry = E->Strings + Index;
        Out(E, "%s:\n.ascii \"", Entry->DataLabel);
        EmitAscii(E, Entry->Value);
        Out(E, "\"\n.byte 0\n");
    }

    for(u32 Index = 0; Index < E->NumberCount; ++Index)
    {
        asm_number *Entry = E->Numbers + Index;
        Out(E, ".align 8\n");
        Out(E, "%s:\n.quad 0x%016llx\n", Entry->Label,
            (unsigned long long)Entry->Bits);
    }

    Out(E, ".data\n");
    for(u32 Index = 0; Index < IR->Semantic->SymbolCount; ++Index)
    {
        semantic_symbol *Symbol = IR->Semantic->Symbols + Index;
        if(Symbol->IsGlobal)
        {
            Out(E, ".align 8\n");
            Out(E, ".LG%u:\n.zero 16\n", Index);
        }
    }
}

internal void
EmitCode(asm_emitter *E, ir_program *IR)
{
    Out(E, ".intel_syntax noprefix\n");
    Out(E, ".extern printf\n");
    Out(E, ".extern memcmp\n");
    Out(E, ".text\n");
    Out(E, ".globl main\n");
    Out(E, ".type main, @function\n");
    Out(E, "main:\n");
    Out(E, "push rbp\n");
    Out(E, "mov rbp, rsp\n");
    Out(E, "sub rsp, 16\n");
    Out(E, "call lua_main\n");
    Out(E, "xor eax, eax\n");
    Out(E, "add rsp, 16\n");
    Out(E, "pop rbp\n");
    Out(E, "ret\n");
    Out(E, ".size main, .-main\n");

    for(u32 Index = 0; Index < IR->InstructionCount; ++Index)
    {
        if(CanEmitIntegerEmptyLoop(E, IR, Index))
        {
            EmitIntegerEmptyLoop(E, IR, Index);
            Index += 3;
        }
        else if(CanFuseCompareJump(E, IR, Index))
        {
            EmitFusedCompareJump(E, IR, Index);
            ++Index;
        }
        else if(CanPackArithmeticPair(E, IR, Index))
        {
            EmitPackedArithmeticPair(E, IR, Index);
            ++Index;
        }
        else if(CanEmitArithmeticChainPair(E, IR, Index))
        {
            EmitArithmeticChainPair(E, IR, Index);
            ++Index;
        }
        else
        {
            EmitInstruction(E, IR, Index);
        }
    }

    Out(E, ".size lua_main, .-lua_main\n");
}

internal void
EmitAssembly(FILE *File, ir_program *IR, memory_arena *Arena)
{
    asm_emitter E = {0};
    E.File = File;
    E.Arena = Arena;

    u32 NodeCount = IR->Parser->StmtCap + IR->Parser->ExprCap + 16;
    E.StringCapacity = NodeCount * 4;
    E.NumberCapacity = NodeCount * 4;
    E.Strings = PushArray(Arena, E.StringCapacity, asm_string);
    E.Numbers = PushArray(Arena, E.NumberCapacity, asm_number);
    E.FrameSizes = PushArray(Arena, IR->FunctionCount, u32);
    E.InstructionCount = IR->InstructionCount;
    E.FunctionEntries = PushArray(Arena, IR->InstructionCount + 1, b32);
    E.JumpTargets = PushArray(Arena, IR->InstructionCount + 1, b32);
    ZeroSize((IR->InstructionCount + 1) * sizeof(*E.FunctionEntries),
             E.FunctionEntries);
    ZeroSize((IR->InstructionCount + 1) * sizeof(*E.JumpTargets),
             E.JumpTargets);

    for(u32 FunctionIndex = 0; FunctionIndex < IR->FunctionCount;
        ++FunctionIndex)
    {
        ir_function_info *Info = IR->Functions + FunctionIndex;
        u32 LocalCount = IR->Semantic->FunctionLocalCounts[FunctionIndex];
        u32 Slots = Info->RegCount + LocalCount;
        E.FrameSizes[FunctionIndex] = Slots * 16;
        if(Info->Entry <= IR->InstructionCount)
        {
            E.FunctionEntries[Info->Entry] = 1;
        }
    }

    for(u32 Index = 0; Index < IR->InstructionCount; ++Index)
    {
        ir_instruction *Instr = IR->Instructions + Index;
        if(((Instr->Op == IR_Jump) || (Instr->Op == IR_JumpIfFalse) ||
            (Instr->Op == IR_JumpIfTrue)) &&
           (Instr->Target <= IR->InstructionCount))
        {
            E.JumpTargets[Instr->Target] = 1;
        }
    }

    EmitCode(&E, IR);
    EmitData(&E, IR);
}
