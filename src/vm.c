#include "vm.h"
#include "env.h"

#include <stdio.h>
#include <stdlib.h>

VM *initVM(u32 register_count)
{
    VM *vm = calloc(1, sizeof(VM));
    if (vm == NULL) {
        fprintf(stderr, "vm: out of memory\n");
        abort();
    }

    vm->register_count = register_count;
    if (register_count > 0) {
        vm->registers = calloc(register_count, sizeof(Literal));
        if (vm->registers == NULL) {
            fprintf(stderr, "vm: out of memory\n");
            abort();
        }
    }

    return vm;
}

void destroyVM(VM *vm)
{
    if (vm == NULL) {
        return;
    }
    free(vm->registers);
    free(vm);
}

Literal getRegister(VM *vm, Register reg)
{
    if (reg >= vm->register_count) {
        fprintf(stderr, "vm: register out of range\n");
        abort();
    }
    return vm->registers[reg];
}

internal void checkRegister(VM *vm, Register reg)
{
    if (reg >= vm->register_count) {
        fprintf(stderr, "vm: register out of range\n");
        abort();
    }
}

void runByteCode(VM *vm, ByteCode *bytecode)
{
    for (u32 pc = 0; pc < bytecode->op_count; ++pc) {
        OpCode op = bytecode->ops[pc];
        checkRegister(vm, op.dest);

        switch (op.type) {
        case LITERAL: {
            vm->registers[op.dest] = op.code.load.value;
        } break;

        case REGISTER: {
            Register a = op.code.arith.a;
            Register b = op.code.arith.b;
            checkRegister(vm, a);
            checkRegister(vm, b);

            Literal x = vm->registers[a];
            Literal y = vm->registers[b];

            switch (op.code.arith.op) {
            case CODE_ADD: {
                vm->registers[op.dest] = x + y;
            } break;
            case CODE_SUB: {
                vm->registers[op.dest] = x - y;
            } break;
            case CODE_MUL: {
                vm->registers[op.dest] = x * y;
            } break;
            case CODE_DIV: {
                vm->registers[op.dest] = x / y;
            } break;
            case CODE_MOV: {
                vm->registers[op.dest] = x;
            } break;
            default: {
                fprintf(stderr, "vm: unknown arithmetic opcode\n");
                abort();
            } break;
            }
        } break;

        default: {
            fprintf(stderr, "vm: unknown opcode type\n");
            abort();
        } break;
        }
    }
}

void applyAssignments(Env *env, ByteCode *bytecode, VM *vm)
{
    for (u32 i = 0; i < bytecode->assign_count; ++i) {
        AssignResult result = bytecode->assigns[i];
        envSet(env, result.name, getRegister(vm, result.reg));
    }
}
