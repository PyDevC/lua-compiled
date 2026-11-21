/* ========================================================================

   $File: ir.h
   $Date: 2026/11/20
   $Revision: 1

   Typed three-address IR. Registers are virtual and every node carries the
   strict type inferred by the semantic pass. Control flow uses instruction
   indices as labels; code generation lowers this directly to amd64 assembly.

   ======================================================================== */

#ifndef LUA_IR_H
#define LUA_IR_H

#include "semantic.h"

#define IRInvalid ((u32)-1)

typedef enum ir_operand_kind
{
    IR_None,
    IR_Reg,
    IR_Const,
    IR_Symbol,
    IR_Function,
} ir_operand_kind;

typedef struct ir_constant
{
    lua_type Type;
    union
    {
        b32 Boolean;
        f64 Number;
        string String;
    };
} ir_constant;

typedef struct ir_operand
{
    ir_operand_kind Kind;
    union
    {
        u32 Reg;
        ir_constant Const;
        u32 SymbolIndex;
        u32 FunctionIndex;
    };
} ir_operand;

typedef enum ir_op
{
    IR_LoadConst,
    IR_LoadFunction,
    IR_LoadSymbol,
    IR_StoreSymbol,
    IR_Move,

    IR_Add,
    IR_Sub,
    IR_Mul,
    IR_Div,
    IR_Neg,
    IR_Not,
    IR_Len,

    IR_Eq,
    IR_Neq,
    IR_Lt,
    IR_Le,
    IR_Gt,
    IR_Ge,

    IR_Jump,
    IR_JumpIfFalse,
    IR_JumpIfTrue,

    IR_Call,
    IR_Return,

    IR_Count,
} ir_op;

typedef struct ir_instruction
{
    ir_op Op;
    lua_type Type;
    ir_operand Dst;
    ir_operand A;
    ir_operand B;

    u32 Target;
    u32 ArgBase;
    u32 ArgCount;

    /* Function whose frame owns the virtual registers in this instruction,
     * and the callee for IR_Call. Keeping them separate is what lets function
     * bodies sit inline in the same instruction array as the main chunk. */
    u32 OwnerFunction;
    u32 CalleeIndex;
} ir_instruction;

typedef struct ir_function_info
{
    u32 Entry;
    u32 RegCount;
    u32 SignatureIndex;
    b32 Emitted;
} ir_function_info;

typedef struct ir_reg_type
{
    u32 FunctionIndex;
    u32 Reg;
    lua_type Type;
} ir_reg_type;

typedef struct ir_program
{
    Parser *Parser;
    semantic_context *Semantic;
    memory_arena *Arena;

    ir_instruction *Instructions;
    u32 InstructionCount;
    u32 InstructionCapacity;

    u32 *Patches;
    u32 PatchCount;
    u32 PatchCapacity;

    /* Break jumps live in their own array: an `if` inside a loop patches the
     * normal patch stack and must never consume a loop's break patches. */
    u32 *BreakPatches;
    u32 BreakCount;
    u32 BreakCapacity;

    u32 *LoopPatchStack;
    u32 LoopCount;
    u32 LoopCapacity;

    ir_function_info *Functions;
    u32 FunctionCount;

    u32 FrameRegCount;
    u32 CurrentFunction;

    /* Strict type per virtual register. Registers are reused across function
     * bodies, so every entry carries its owning function. The newest entry for
     * a (function, register) pair wins. */
    ir_reg_type *RegTypes;
    u32 RegTypeCapacity;
    u32 RegTypeCount;
} ir_program;

internal void InitIR(ir_program *IR, Parser *Parser,
                     semantic_context *Semantic, memory_arena *Arena);
internal void GenerateIR(ir_program *IR, ASTIndex FirstStatement);
internal void OptimizeIR(ir_program *IR);
internal void DumpIR(ir_program *IR);

internal char *IROpName(ir_op Op);
internal lua_type GetRegType(ir_program *IR, u32 FunctionIndex, u32 Reg);

#endif /* LUA_IR_H */
