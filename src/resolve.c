#include "resolve.h"
#include "errors.h"

SymType get_expression_type(ExprNode *expression, SymTable *STable);

void resolve_node_stat_list(StatNodeList *block, SymTable *STable)
{
    StatNodeList *temp = block;

    SymTable *Inner = STable_create(STable);
    while (temp != NULL) {
        resolve_node_stat(temp->stat, Inner);
        temp = temp->next;
    }
}

void resolve_node_stat(StatNode *statement, SymTable *STable)
{
    switch (statement->type) {
    case AssignmentStat: {
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
    Symbol *sym = Sym_create(S_NIL, (void *)variable);
    STable_set_entry(STable, variable->name, sym);
}

void resolve_node_expr(ExprNode *expression, SymTable *STable, const char *key)
{
    Symbol *sym = STable_get_value(STable, key);
    if (sym == NULL) {
        E(fprintf(stderr, "Syntax Error: Variable not declared\n"));
    }

    sym->type = get_expression_type(expression, STable);
}

SymType get_expression_type(ExprNode *expression, SymTable *STable)
{
    switch (expression->type) {
    case NumberExpr: {
        return S_FLOAT;
    } break;
    case StringExpr: {
        return S_STRING;
    } break;
    case NilExpr: {
        return S_NIL;
    } break;
    case BooleanExpr: {
        return S_BOOLEAN;
    } break;
    case VariableExpr: {
        Symbol *sym = STable_get_value(STable, expression->data.var->name);
        if (sym == NULL) {
            E(fprintf(stderr, "Syntax Error: Variable not declared\n"));
        }
        return sym->type;
    } break;
    case BinaryExpr: {
        // TODO: do SymType conversions in SymTable to prevent IR to generate
        // Invalid Operation
        switch (expression->data.binary_expr.op) {
        case ADD:
        case SUB:
        case MUL:
        case DIV: {
            SymType left =
                get_expression_type(expression->data.binary_expr.left, STable);
            SymType right =
                get_expression_type(expression->data.binary_expr.right, STable);
            if (left == S_INT) {
                if (right == S_INT) {
                    return S_INT;
                } else if (right == S_FLOAT) {
                    return S_FLOAT;
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between an integer and a string\n"));
                } else if (right == S_BOOLEAN) {
                    return S_INT;
                } else if (right == S_NIL) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between an integer and a nil\n"));
                }
            } else if (left == S_FLOAT) {
                if (right == S_INT) {
                    return S_FLOAT;
                } else if (right == S_FLOAT) {
                    return S_FLOAT;
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a float and a string\n"));
                } else if (right == S_BOOLEAN) {
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
            } else if (S_BOOLEAN) {
                if (right == S_INT) {
                    return S_INT;
                } else if (right == S_FLOAT) {
                    return S_FLOAT;
                } else if (right == S_STRING) {
                    E(fprintf(
                        stderr,
                        "Invalid Operation between a boolean and a string\n"));
                } else if (right == S_BOOLEAN) {
                    return S_BOOLEAN;
                } else if (right == S_NIL) {
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
                    return S_BOOLEAN;
                } else if (right == S_NIL) {
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
