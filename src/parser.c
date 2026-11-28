#include "parser.h"
#include <assert.h>
#include <string.h>

#define IsBlockEnd(x)                                                          \
    ((x == TK_END) || (x == TK_ELSE) || (x == TK_ELSEIF) || (x == TK_UNTIL) || \
     (x == TK_EOF))

u32 smartPoolGenerator(size_t fsize) { return (u32)fsize * 3; }

internal AST_INDEX allocExpr(Parser *parser, Expr node)
{
    if (parser->expr_len >= parser->expr_capacity) {
        parser->expr_capacity =
            parser->expr_capacity == 0
                ? smartPoolGenerator(parser->lex_state->fbuffer->fsize)
                : 2 * parser->expr_capacity;
        Expr *new_pool =
            realloc(parser->expr_pool, parser->expr_capacity * sizeof(Expr));
        if (!new_pool) return AST_NONE;
        parser->expr_pool = new_pool;
    }
    node.next = AST_NONE;
    parser->expr_pool[parser->expr_len] = node;
    return parser->expr_len++;
}

internal AST_INDEX allocStmt(Parser *parser, Statement stmt)
{
    if (parser->stmt_len >= parser->stmt_capacity) {
        parser->stmt_capacity =
            parser->stmt_capacity == 0
                ? smartPoolGenerator(parser->lex_state->fbuffer->fsize)
                : 2 * parser->stmt_capacity;
        Statement *new_pool = realloc(parser->stmt_pool, parser->stmt_capacity *
                                                             sizeof(Statement));
        if (!new_pool) return AST_NONE;
        parser->stmt_pool = new_pool;
    }
    stmt.next = AST_NONE;
    parser->stmt_pool[parser->stmt_len] = stmt;
    return parser->stmt_len++;
}

void pAdvance(Parser *parser)
{
    parser->previous = parser->current;
    parser->current = lConsume(parser->lex_state);
}

int pMatch(Parser *parser, ReservedToken tok_type)
{
    if (parser->current.tok_type == tok_type) {
        pAdvance(parser);
        return 1;
    }
    return 0;
}

typedef AST_INDEX (*PrefixFn)(Parser *parser);
typedef AST_INDEX (*InfixFn)(Parser *parser, AST_INDEX left);

typedef struct
{
    PrefixFn prefix;
    InfixFn infix;
    Precedence precedence;
} ParserRule;

extern const ParserRule rules[];

AST_INDEX parseExpression(Parser *parser, Precedence precedence)
{
    PrefixFn prefix_rule = rules[parser->current.tok_type].prefix;
    if (!prefix_rule) return AST_NONE;

    pAdvance(parser);
    AST_INDEX left = prefix_rule(parser);
    while (precedence <= rules[parser->current.tok_type].precedence) {
        InfixFn infix_rule = rules[parser->current.tok_type].infix;
        if (!infix_rule) return AST_NONE;
        pAdvance(parser);
        left = infix_rule(parser, left);
    }

    return left;
}

AST_INDEX parsePrimary(Parser *parser)
{
    const char *variable;
    switch (parser->previous.tok_type) {
    case TK_NIL:
        return allocExpr(parser, (Expr){.type = ExprNil, .as = {NULL}});
    case TK_NUMBER:
        return allocExpr(parser,
                         (Expr){.type = ExprNumber,
                                .as.number = parser->previous.info.number});
    case TK_TRUE:
        return allocExpr(parser,
                         (Expr){.type = ExprBoolean, .as.boolean = true});
    case TK_FALSE:
        return allocExpr(parser,
                         (Expr){.type = ExprBoolean, .as.boolean = false});
    case TK_NAME:
        variable = parser->previous.info.str;
        return allocExpr(parser,
                         (Expr){.type = ExprVariable, .as.variable = variable});
    case TK_STRING:
        variable = parser->previous.info.str;
        return allocExpr(parser,
                         (Expr){.type = ExprString, .as.string = variable});
    default:
        return AST_NONE;
    };
}

AST_INDEX parseBinary(Parser *parser, AST_INDEX left)
{
    ReservedToken op_type = parser->previous.tok_type;
    Precedence precedence = rules[op_type].precedence;
    AST_INDEX right = parseExpression(parser, (Precedence)(precedence + 1));
    return allocExpr(parser, (Expr){.type = ExprBinary,
                                    .as.binary.l_expr = left,
                                    .as.binary.r_expr = right,
                                    .as.binary.op = op_type});
}

AST_INDEX parseUnary(Parser *parser)
{
    ReservedToken op_type = parser->previous.tok_type;
    Precedence precedence = rules[op_type].precedence;
    AST_INDEX left = parseExpression(parser, (Precedence)(precedence + 1));
    return allocExpr(parser, (Expr){.type = ExprUnary,
                                    .as.unary.l_expr = left,
                                    .as.unary.op = op_type});
}

AST_INDEX parseGroup(Parser *parser)
{
    AST_INDEX group_expr = parseExpression(parser, PREC_ASSIGN);

    if (pMatch(parser, TK_RPAREN)) {
        return allocExpr(parser,
                         (Expr){.type = ExprGroup, .as.group = group_expr});
    }
    return AST_NONE;
}

AST_INDEX parseCall(Parser *parser, AST_INDEX left)
{
    AST_INDEX first = AST_NONE;
    AST_INDEX last = AST_NONE;
    u32 argc = 0;

    if (parser->current.tok_type != TK_RPAREN) {
        do {
            AST_INDEX arg = parseExpression(parser, PREC_ASSIGN);
            if (arg == AST_NONE) return AST_NONE;
            if (first == AST_NONE) {
                first = arg;
            } else {
                parser->expr_pool[last].next = arg;
            }
            last = arg;
            argc++;
        } while (pMatch(parser, TK_COMMA));
    }
    if (!pMatch(parser, TK_RPAREN)) return AST_NONE;

    return allocExpr(parser, (Expr){.type = ExprFuncCall,
                                    .as.call.callee = left,
                                    .as.call.argv = first,
                                    .as.call.argc = argc});
}

const ParserRule rules[] = {
    /* Constants  */
    [TK_NIL] = {parsePrimary, NULL, PREC_NONE},
    [TK_NUMBER] = {parsePrimary, NULL, PREC_NONE},
    [TK_STRING] = {parsePrimary, NULL, PREC_NONE},
    [TK_NAME] = {parsePrimary, NULL, PREC_NONE},
    [TK_TRUE] = {parsePrimary, NULL, PREC_NONE},
    [TK_FALSE] = {parsePrimary, NULL, PREC_NONE},

    /* arithmetic expressions */
    [TK_ADD] = {NULL, parseBinary, PREC_TERM},
    [TK_SUB] = {parseUnary, parseBinary, PREC_TERM},
    [TK_MUL] = {NULL, parseBinary, PREC_FACTOR},
    [TK_DIV] = {NULL, parseBinary, PREC_FACTOR},

    /* comparsion expressions */
    [TK_EQ_EQ] = {NULL, parseBinary, PREC_COMPARE},
    [TK_NEQ] = {NULL, parseBinary, PREC_COMPARE},
    [TK_LT] = {NULL, parseBinary, PREC_COMPARE},
    [TK_LT_EQ] = {NULL, parseBinary, PREC_COMPARE},
    [TK_GT] = {NULL, parseBinary, PREC_COMPARE},
    [TK_GT_EQ] = {NULL, parseBinary, PREC_COMPARE},

    [TK_LPAREN] = {parseGroup, parseCall, PREC_CALL},
    [TK_RPAREN] = {NULL, NULL, PREC_NONE},
    [TK_EOF] = {NULL, NULL, PREC_NONE},
};

AST_INDEX parseAssignmentStatement(Parser *parser)
{
    AST_INDEX target = parseExpression(parser, PREC_CALL);
    if (target == AST_NONE) return AST_NONE;

    if (pMatch(parser, TK_EQ)) {
        if (getExpression(parser, target)->type != ExprVariable)
            return AST_NONE;
        const char *name = getExpression(parser, target)->as.variable;
        AST_INDEX value = parseExpression(parser, PREC_ASSIGN);
        return allocStmt(parser, (Statement){.type = StmtAssignment,
                                             .as.assignment.name = name,
                                             .as.assignment.value = value});
    }

    if (getExpression(parser, target)->type != ExprFuncCall) return AST_NONE;

    return allocStmt(
        parser, (Statement){.type = StmtExpr, .as.expr_stmt.expr = target});
}

AST_INDEX parseLocalStatement(Parser *parser)
{
    pAdvance(parser);
    if (pMatch(parser, TK_NAME)) {
        const char *name = parser->previous.info.str;
        AST_INDEX value = AST_NONE;
        if (pMatch(parser, TK_EQ)) {
            value = parseExpression(parser, PREC_ASSIGN);
        }
        return allocStmt(parser, (Statement){.type = StmtLocalVar,
                                             .as.local_var.name = name,
                                             .as.local_var.value = value});
    } else {
        return AST_NONE;
    }
}

AST_INDEX parseIfThenBlock(Parser *parser)
{
    AST_INDEX expr_idx = parseExpression(parser, PREC_ASSIGN);
    AST_INDEX block_idx = AST_NONE;

    if (pMatch(parser, TK_THEN)) {
        block_idx = parseBlock(parser);
    } else {
        // error
    }

    AST_INDEX stmt_idx = allocStmt(
        parser, (Statement){.type = StmtIfElse,
                            .as.if_else_stmt.condition_expr = expr_idx,
                            .as.if_else_stmt.block_idx = block_idx,
                            .as.if_else_stmt.next_elif_or_else = AST_NONE});
    return stmt_idx;
}

AST_INDEX parseElseBlock(Parser *parser)
{
    AST_INDEX block_idx = parseBlock(parser);

    AST_INDEX stmt_idx = allocStmt(
        parser, (Statement){.type = StmtIfElse,
                            .as.if_else_stmt.condition_expr = AST_NONE,
                            .as.if_else_stmt.block_idx = block_idx,
                            .as.if_else_stmt.next_elif_or_else = AST_NONE});
    return stmt_idx;
}

AST_INDEX parseIfStatement(Parser *parser)
{
    pAdvance(parser);
    AST_INDEX if_stmt_idx = parseIfThenBlock(parser);

    AST_INDEX elseif_prev_stmt_idx = AST_NONE;
    AST_INDEX elseif_latest_stmt_idx = AST_NONE;

    while (pMatch(parser, TK_ELSEIF)) {
        if (elseif_prev_stmt_idx == AST_NONE) {
            elseif_latest_stmt_idx = parseIfThenBlock(parser);
            Statement *if_stmt = getStatement(parser, if_stmt_idx);
            if_stmt->as.if_else_stmt.next_elif_or_else = elseif_latest_stmt_idx;
        } else {
            elseif_latest_stmt_idx = parseIfThenBlock(parser);
            Statement *elseif_prev_stmt =
                getStatement(parser, elseif_prev_stmt_idx);
            elseif_prev_stmt->as.if_else_stmt.next_elif_or_else =
                elseif_latest_stmt_idx;
        }
        elseif_prev_stmt_idx = elseif_latest_stmt_idx;
    }

    if (pMatch(parser, TK_ELSE)) {
        AST_INDEX else_stmt = parseElseBlock(parser);
        if (elseif_prev_stmt_idx == AST_NONE) {
            Statement *if_stmt = getStatement(parser, if_stmt_idx);
            if_stmt->as.if_else_stmt.next_elif_or_else = else_stmt;
        } else {
            Statement *elseif_latest_stmt =
                getStatement(parser, elseif_latest_stmt_idx);
            elseif_latest_stmt->as.if_else_stmt.next_elif_or_else = else_stmt;
        }
    }

    if (!pMatch(parser, TK_END)) {
        // error
    }

    return if_stmt_idx;
}

AST_INDEX parseWhileLoopStmt(Parser *parser)
{
    pAdvance(parser);
    AST_INDEX while_expr = parseExpression(parser, PREC_ASSIGN);
    AST_INDEX while_block = parseBlock(parser);
    if (!pMatch(parser, TK_END)) {
        // error
    }

    return allocStmt(
        parser,
        (Statement){.type = StmtWhile,
                    .as.while_or_repeat_stmt.condition_expr = while_expr,
                    .as.while_or_repeat_stmt.block_idx = while_block,
                    .as.while_or_repeat_stmt.is_repat_until = false});
}

AST_INDEX parseRepeatUntilStmt(Parser *parser)
{
    pAdvance(parser);
    AST_INDEX repeat_block = parseBlock(parser);
    if (!pMatch(parser, TK_UNTIL)) {
        // error
    }
    AST_INDEX until_expr = parseExpression(parser, PREC_ASSIGN);

    return allocStmt(
        parser,
        (Statement){.type = StmtRepeatUntil,
                    .as.while_or_repeat_stmt.condition_expr = until_expr,
                    .as.while_or_repeat_stmt.block_idx = repeat_block,
                    .as.while_or_repeat_stmt.is_repat_until = true});
}

AST_INDEX parseForLoopStmt(Parser *parser)
{
    pAdvance(parser);
    if (!pMatch(parser, TK_NAME)) {
        // error
    }

    const char *var = parser->previous.info.str;
    if (!pMatch(parser, TK_EQ)) {
        // error
    }

    AST_INDEX start_expr = parseExpression(parser, PREC_ASSIGN);
    if (!pMatch(parser, TK_COMMA)) {
        // error
    }

    AST_INDEX end_expr = parseExpression(parser, PREC_ASSIGN);
    AST_INDEX step_expr = AST_NONE;
    if (pMatch(parser, TK_COMMA)) {
        step_expr = parseExpression(parser, PREC_ASSIGN);
    }

    if (!pMatch(parser, TK_DO)) {
        // error
    }

    AST_INDEX for_block = parseBlock(parser);
    if (!pMatch(parser, TK_END)) {
        // error
    }

    return allocStmt(parser, (Statement){.type = StmtForLoop,
                                         .as.for_stmt.var = var,
                                         .as.for_stmt.start_expr = start_expr,
                                         .as.for_stmt.end_expr = end_expr,
                                         .as.for_stmt.step_expr = step_expr,
                                         .as.for_stmt.block_idx = for_block});
}

AST_INDEX parseFunctionStmt(Parser *parser)
{
    pAdvance(parser);
    if (!pMatch(parser, TK_NAME)) {
        // error
        return AST_NONE;
    }

    const char *name = parser->previous.info.str;
    if (!pMatch(parser, TK_LPAREN)) {
        // error
        return AST_NONE;
    }

    AST_INDEX first = AST_NONE;
    AST_INDEX last = AST_NONE;
    u32 argc = 0;

    if (parser->current.tok_type != TK_RPAREN) {
        do {
            AST_INDEX arg = parseExpression(parser, PREC_ASSIGN);
            if (arg == AST_NONE) return AST_NONE;
            if (first == AST_NONE) {
                first = arg;
            } else {
                parser->expr_pool[last].next = arg;
            }
            last = arg;
            argc++;
        } while (pMatch(parser, TK_COMMA));
    }
    if (!pMatch(parser, TK_RPAREN)) return AST_NONE;

    AST_INDEX block_idx = parseBlock(parser);
    if (!pMatch(parser, TK_END)) { // error
    }
    return allocStmt(parser, (Statement){.type = StmtFunction,
                                         .as.func_stmt.name = name,
                                         .as.func_stmt.argv = first,
                                         .as.func_stmt.argc = argc,
                                         .as.func_stmt.block_idx = block_idx});
}

AST_INDEX parseStatement(Parser *parser)
{
    switch (parser->current.tok_type) {
    case TK_NAME:
        return parseAssignmentStatement(parser);
    case TK_LOCAL:
        return parseLocalStatement(parser);
    case TK_IF:
        return parseIfStatement(parser);
    case TK_WHILE:
        return parseWhileLoopStmt(parser);
    case TK_REPEAT:
        return parseRepeatUntilStmt(parser);
    case TK_FOR:
        return parseForLoopStmt(parser);
    case TK_FUNCTION:
        return parseFunctionStmt(parser);
    default:
        pAdvance(parser);
        return AST_NONE;
    };
}

AST_INDEX parseBlock(Parser *parser)
{
    AST_INDEX first = AST_NONE;
    AST_INDEX last = AST_NONE;
    while (!IsBlockEnd(parser->current.tok_type) &&
           !(parser->current.tok_type == TK_ILLEGAL)) {
        AST_INDEX stmt = parseStatement(parser);
        if (stmt == AST_NONE) continue;
        if (first == AST_NONE) {
            first = stmt;
        } else {
            parser->stmt_pool[last].next = stmt;
        }
        last = stmt;
    }

    return allocStmt(parser, (Statement){.type = StmtBlock,
                                         .as.block.block_idx = first,
                                         .next = AST_NONE});
}

Statement *getStatement(Parser *parser, AST_INDEX stmt)
{
    assert(stmt != AST_NONE);
    return &parser->stmt_pool[stmt];
}

Expr *getExpression(Parser *parser, AST_INDEX expr)
{
    assert(expr != AST_NONE);
    return &parser->expr_pool[expr];
}

Parser *initParser(const char *name)
{
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->lex_state = initLexer(name);
    parser->expr_pool = NULL;
    parser->expr_len = 1;
    parser->expr_capacity = 0;
    parser->stmt_pool = NULL;
    parser->stmt_len = 1;
    parser->stmt_capacity = 0;
    parser->current = lConsume(parser->lex_state);
    return parser;
}

void destroyParser(Parser *parser)
{
    free(parser->expr_pool);
    free(parser->stmt_pool);
    destroyLexer(parser->lex_state);
    free(parser);
}

#ifdef PARSER_DEBUG

void printExpression(Parser *parser, Expr *expr)
{
    switch (expr->type) {
    case ExprNil: {
        printf("nil ");
    } break;
    case ExprBoolean: {
        printf("%b ", expr->as.boolean);
    } break;
    case ExprNumber: {
        printf("%f ", expr->as.number);
    } break;
    case ExprString: {
        printf("%s ", expr->as.string);
    } break;
    case ExprVariable: {
        printf("%s ", expr->as.variable);
    } break;
    case ExprBinary: {
        printExpression(parser, getExpression(parser, expr->as.binary.l_expr));
        printExpression(parser, getExpression(parser, expr->as.binary.r_expr));
        printToken(expr->as.binary.op);
        printf(" ");
    } break;
    case ExprGroup: {
        printf("( ");
        printExpression(parser, getExpression(parser, expr->as.group));
        printf(") ");
    } break;
    default:
        printf("unsupported");
    };
}

void printStatement(Parser *parser, Statement *stmt)
{
    switch (stmt->type) {
    case StmtBlock: {
        AST_INDEX stmt_idx = stmt->as.block.block_idx;
        printf("Block:\n");
        while (stmt_idx != AST_NONE) {
            stmt = getStatement(parser, stmt_idx);
            printStatement(parser, stmt);
            printf("\n");
            stmt_idx = stmt->next;
        }
        printf("end\n");
    } break;
    case StmtLocalVar: {
        printf("local ");
        printf("%s = ", stmt->as.local_var.name);
        printExpression(parser,
                        getExpression(parser, stmt->as.local_var.value));
    } break;
    case StmtAssignment: {
        printf("%s = ", stmt->as.assignment.name);
        printExpression(parser,
                        getExpression(parser, stmt->as.assignment.value));
    } break;
    case StmtIfElse: {
        printf("if (");
        printExpression(
            parser,
            getExpression(parser, stmt->as.if_else_stmt.condition_expr));
        printf(") then\n");
        Statement *block =
            getStatement(parser, stmt->as.if_else_stmt.block_idx);
        printStatement(parser, block);
        if (stmt->as.if_else_stmt.next_elif_or_else != AST_NONE) {
            Statement *elif_or_else =
                getStatement(parser, stmt->as.if_else_stmt.next_elif_or_else);
            if (elif_or_else) {
                printf("else or elif");
                printStatement(parser, elif_or_else);
            }
        }
    } break;
    case StmtWhile: {
        assert(!stmt->as.while_or_repeat_stmt.is_repat_until);
        printf("while (");
        printExpression(
            parser, getExpression(
                        parser, stmt->as.while_or_repeat_stmt.condition_expr));
        printf(") do\n");
        Statement *block =
            getStatement(parser, stmt->as.while_or_repeat_stmt.block_idx);
        printStatement(parser, block);
    } break;
    case StmtRepeatUntil: {
        assert(stmt->as.while_or_repeat_stmt.is_repat_until);
        printf("repeat until (");
        printExpression(
            parser, getExpression(
                        parser, stmt->as.while_or_repeat_stmt.condition_expr));
        printf(") do\n");
        Statement *block =
            getStatement(parser, stmt->as.while_or_repeat_stmt.block_idx);
        printStatement(parser, block);
    } break;
    case StmtForLoop: {
        printf("for %s = ", stmt->as.for_stmt.var);
        printExpression(parser,
                        getExpression(parser, stmt->as.for_stmt.start_expr));
        printf(", ");
        printExpression(parser,
                        getExpression(parser, stmt->as.for_stmt.end_expr));
        printf(", ");
        printExpression(parser,
                        getExpression(parser, stmt->as.for_stmt.step_expr));
        printf("do\n");
        printStatement(parser,
                       getStatement(parser, stmt->as.for_stmt.block_idx));
    } break;
    case StmtFunction: {
        printf("function %s(", stmt->as.func_stmt.name);
        if (stmt->as.func_stmt.argc > 0) {
            AST_INDEX arg_idx = stmt->as.func_stmt.argv;
            while (arg_idx != AST_NONE) {
                Expr *arg = getExpression(parser, arg_idx);
                printExpression(parser, arg);
                printf(", ");
                arg_idx = arg->next;
            }
        }
        printf(")\n");
        printStatement(parser,
                       getStatement(parser, stmt->as.func_stmt.block_idx));

    } break;
    case StmtFuncCall: {
        printExpression(parser,
                        getExpression(parser, stmt->as.funccall.callee));
    } break;
    case StmtExpr: {
        printExpression(parser, getExpression(parser, stmt->as.expr_stmt.expr));
    } break;
    };
}

#endif // PARSER_DEBUG
