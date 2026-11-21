/* ========================================================================

   $File: x86.h
   $Date: 2026/11/20
   $Revision: 1

   AMD64 textual assembly backend. There is no interpreter between the typed
   IR and the machine: every IR instruction lowers to a short sequence of
   real instructions in a .s file that the system assembler turns into a
   native executable.

   ======================================================================== */

#ifndef LUA_X86_H
#define LUA_X86_H

#include "ir.h"

typedef struct asm_string
{
    string Value;
    char *DataLabel;
} asm_string;

typedef struct asm_number
{
    u64 Bits;
    char *Label;
} asm_number;

typedef struct asm_emitter
{
    FILE *File;
    memory_arena *Arena;

    asm_string *Strings;
    u32 StringCount;
    u32 StringCapacity;

    asm_number *Numbers;
    u32 NumberCount;
    u32 NumberCapacity;

    u32 Unique;
    u32 *FrameSizes;
    u32 InstructionCount;
    b32 *FunctionEntries;
    b32 *JumpTargets;
} asm_emitter;

internal void EmitAssembly(FILE *File, ir_program *IR, memory_arena *Arena);

#endif /* LUA_X86_H */
