#pragma once

#include "ir.h"

typedef struct VM
{
    Literal *registers;
    u32 register_count;
} VM;

VM *initVM(u32 register_count);
void destroyVM(VM *vm);
void runByteCode(VM *vm, ByteCode *bytecode);
void applyAssignments(Env *env, ByteCode *bytecode, VM *vm);
Literal getRegister(VM *vm, Register reg);
