#ifndef IR_H
#define IR_H

typedef enum { OP_VAR, OP_CONST, OP_TEMP } OperandType;

typedef struct
{
    OperandType type;
    union
    {
        char *name; // For OP_VAR and OP_TEMP
        double val; // For OP_CONST
    } data;
} IROperand;

typedef enum { IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_ASSIGN } IROp;

typedef struct IRInstr
{
    IROp op;
    IROperand dst;
    IROperand src1;
    IROperand src2;
    struct IRInstr *next; // Linked list for the instruction stream
} IRInstr;

#endif // IR_H
