#pragma once

#include "defs.h"
#include "parser.h"

typedef u32 Register;
typedef double Literal;

typedef struct Env Env;

typedef enum { REGISTER, LITERAL } CodeType;

typedef enum {
    CODE_UNKNOWN,
    CODE_ADD,
    CODE_SUB,
    CODE_MUL,
    CODE_DIV,
    CODE_MOV
} ArithOp;

typedef struct OpCode
{
    CodeType type;
    Register dest;
    union
    {
        struct
        {
            ArithOp op;
            Register a;
            Register b;
        } arith;

        struct
        {
            Literal value;
        } load;
    } code;
} OpCode;

typedef struct
{
    const char *name;
    Register reg;
} AssignResult;

typedef struct ByteCode
{
    OpCode *ops;
    u32 op_count;
    u32 op_cap;
    Register reg;
    AssignResult *assigns;
    u32 assign_count;
    u32 assign_cap;
} ByteCode;

ByteCode *emitByteCode(Parser *parser, Statement *stmt, Env *env);
void destroyByteCode(ByteCode *bytecode);
void printByteCode(ByteCode *bytecode);
