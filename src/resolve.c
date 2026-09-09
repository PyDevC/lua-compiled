#include "resolve.h"
#include "errors.h"

SymType get_expression_type(ExprNode *expression, SymTable *STable);

void resolve_node_stat_list(StatNodeList *block, SymTable *STable)
{
    StatNodeList *temp = block;

    SymTable *Inner = STable_create(STable);
    D(fprintf(stdout, "DEBUG: Entered the inner scope\n"));
    while (temp != NULL) {
        resolve_node_stat(temp->stat, Inner);
        temp = temp->next;
    }
}

void resolve_node_stat(StatNode *statement, SymTable *STable)
{
    switch (statement->type) {
    case AssignmentStat: {
        D(fprintf(stdout, "DEBUG: AssignmentStat resolve var and expr\n"));
        resolve_node_var(statement->data.assingment_stat.var, STable);
        resolve_node_expr(statement->data.assingment_stat.expr, STable,
                          statement->data.assingment_stat.var->name);
    } break;
    default:
        break;
    }
}

void resolve_node_var(VarNode *variable, SymTable *STable)
{
    Symbol *sym = STable_get_value(STable, variable->name);
    if (sym == NULL) {
        sym = Sym_create(S_NIL, (void *)variable);
        D(fprintf(stdout, "DEBUG: Set Variable: %s as NIL\n", variable->name));
        STable_set_entry(STable, variable->name, sym);
    } else {
        D(fprintf(stdout,
                  "DEBUG: Already declared Variable: %s as SymType: %d\n",
                  variable->name, sym->type));
    }
}

void resolve_node_expr(ExprNode *expression, SymTable *STable, const char *key)
{
    Symbol *sym = STable_get_value(STable, key);
    if (sym == NULL) {
        E(fprintf(stderr, "Syntax Error: Variable not declared\n"));
    }

    sym->type = get_expression_type(expression, STable);
    STable_set_entry(STable, key, sym);
    sym = STable_get_value(STable, key);
    D(fprintf(stdout, "DEBUG: Set Variable: %s as SymType: %d\n", key,
              sym->type));
}

SymType get_expression_type(ExprNode *expression, SymTable *STable)
{
    switch (expression->type) {
    case NumberExpr: {
        D(fprintf(stdout, "DEBUG: Expression %lf is float\n",
                  expression->data.literalnumber));
        return S_FLOAT;
    } break;
    case StringExpr: {
        D(fprintf(stdout, "DEBUG: Expression %s is string\n",
                  expression->data.literalstring));
        return S_STRING;
    } break;
    case NilExpr: {
        D(fprintf(stdout, "DEBUG: Expression %d is nil\n",
                  expression->data.isnil));
        return S_NIL;
    } break;
    case BooleanExpr: {
        D(fprintf(stdout, "DEBUG: Expression %d is Boolean\n",
                  expression->data.boolean));
        return S_BOOLEAN;
    } break;
    case VariableExpr: {
        Symbol *sym = STable_get_value(STable, expression->data.var->name);
        if (sym == NULL) {
            E(fprintf(stderr, "Syntax Error: Variable not declared\n"));
        }
        D(fprintf(stdout, "DEBUG: Variable Expression %s is %d\n",
                  expression->data.var->name, sym->type));
        return sym->type;
    } break;
    case BinaryExpr: {
        // TODO: do SymType conversions in SymTable to prevent IR to generate
        // Invalid Operation
        switch (expression->data.binary_expr.op.type) {
        case ADD:
        case SUB:
        case MUL:
        case DIV: {
            D(fprintf(stdout, "DEBUG: Binary Expression resolve left\n"));
            SymType left =
                get_expression_type(expression->data.binary_expr.left, STable);
            D(fprintf(stdout, "DEBUG: Binary Expression resolve right\n"));
            SymType right =
                get_expression_type(expression->data.binary_expr.right, STable);
            if (left == S_INT) {
                if (right == S_INT) {
                    D(fprintf(stdout, "DEBUG: integer + integer = integer\n"));
                    return S_INT;
                } else if (right == S_FLOAT) {
                    D(fprintf(stdout, "DEBUG: integer + float = float \n"));
                    return S_FLOAT;
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between an integer and a string\n"));
                } else if (right == S_BOOLEAN) {
                    D(fprintf(stdout, "DEBUG: integer + boolean = integer\n"));
                    return S_INT;
                } else if (right == S_NIL) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between an integer and a nil\n"));
                }
            } else if (left == S_FLOAT) {
                if (right == S_INT) {
                    D(fprintf(stdout, "DEBUG: float + integer = float\n"));
                    return S_FLOAT;
                } else if (right == S_FLOAT) {
                    D(fprintf(stdout, "DEBUG: float + float = float\n"));
                    return S_FLOAT;
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a float and a string\n"));
                } else if (right == S_BOOLEAN) {
                    D(fprintf(stdout, "DEBUG: float + boolean = float\n"));
                    return S_FLOAT;
                } else if (right == S_NIL) {
                    E(fprintf(stderr,
                              "Invalid Operation between a float and a nil\n"));
                }
            } else if (left == S_STRING) {
                if (right == S_INT) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a string and an integer\n"));
                } else if (right == S_FLOAT) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a string and a float\n"));
                } else if (right == S_STRING) {
                    D(fprintf(stdout, "DEBUG: string + string = string\n"));
                    return S_STRING;
                } else if (right == S_BOOLEAN) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a string and a boolean\n"));
                } else if (right == S_NIL) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a string and a nil\n"));
                }
            } else if (left == S_BOOLEAN) {
                if (right == S_INT) {
                    D(fprintf(stdout, "DEBUG: boolean + integer = integer\n"));
                    return S_INT;
                } else if (right == S_FLOAT) {
                    D(fprintf(stdout, "DEBUG: boolean + float = float\n"));
                    return S_FLOAT;
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a boolean and a string\n"));
                } else if (right == S_BOOLEAN) {
                    D(fprintf(stdout, "DEBUG: boolean + boolean = boolean\n"));
                    return S_BOOLEAN;
                } else if (right == S_NIL) {
                    D(fprintf(stdout, "DEBUG: boolean + nil = boolean\n"));
                    return S_BOOLEAN;
                }
            } else if (left == S_NIL) {
                if (right == S_INT) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a nil and an integer\n"));
                } else if (right == S_FLOAT) {
                    E(fprintf(stderr,
                              "Invalid Operation between a nil and a float\n"));
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a nil and a string\n"));
                } else if (right == S_BOOLEAN) {
                    D(fprintf(stdout, "DEBUG: nil + boolean = boolean\n"));
                    return S_BOOLEAN;
                } else if (right == S_NIL) {
                    D(fprintf(stdout, "DEBUG: nil + nil = nil\n"));
                    return S_NIL;
                }
            }
        } break;
        default:
            E(fprintf(stderr, "Syntax Error: Invalid Operation\n"));
            break;
        }
    } break;
    default:
        break;
    };
    return S_NIL;
}
