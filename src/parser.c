#include "parser.h"
#include <string.h>

#define is_block_end(x) ((x == TK_END) || (x == TK_ELSE) || (x == TK_EOF))

static ASTIndex alloc_expr(Parser *parser, Expr node)
{
    if (parser->expr_len >= parser->expr_capacity) {
        parser->expr_capacity =
            parser->expr_capacity == 0 ? 256 : 2 * parser->expr_capacity;
        Expr *new_pool =
            realloc(parser->expr_pool, parser->expr_capacity * sizeof(Expr));
        if (!new_pool) return ASTNone;
        parser->expr_pool = new_pool;
    }
    parser->expr_pool[parser->expr_len] = node;
    return parser->expr_len++;
}

static ASTIndex alloc_stmt(Parser *parser, Statement stmt)
{
    if (parser->stmt_len >= parser->stmt_capacity) {
        parser->stmt_capacity =
            parser->stmt_capacity == 0 ? 256 : 2 * parser->stmt_capacity;
        Statement *new_pool = realloc(parser->stmt_pool, parser->stmt_capacity *
                                                             sizeof(Statement));
        if (!new_pool) return ASTNone;
        parser->stmt_pool = new_pool;
    }
    stmt.next = ASTNone;
    parser->stmt_pool[parser->stmt_len] = stmt;
    return parser->stmt_len++;
}

void p_advance(Parser *parser)
{
    parser->previous = parser->current;
    parser->current = lconsume(parser->lex_state);
}

int p_match(Parser *parser, ReservedToken tok_type)
{
    if (parser->current.tok_type == tok_type) {
        p_advance(parser);
        return 1;
    }
    return 0;
}

typedef ASTIndex (*PrefixFn)(Parser *parser);
typedef ASTIndex (*InfixFn)(Parser *parser, ASTIndex left);

typedef struct
{
    PrefixFn prefix;
    InfixFn infix;
    Precedence precedence;
} ParserRule;

extern const ParserRule rules[];

ASTIndex parse_expression(Parser *parser, Precedence precedence)
{
    PrefixFn prefix_rule = rules[parser->current.tok_type].prefix;
    if (!prefix_rule) return ASTNone;

    p_advance(parser);
    ASTIndex left = prefix_rule(parser);
    while (precedence <= rules[parser->current.tok_type].precedence) {
        InfixFn infix_rule = rules[parser->current.tok_type].infix;
        if (!infix_rule) return ASTNone;
        p_advance(parser);
        left = infix_rule(parser, left);
    }

    return left;
}

ASTIndex parse_primary(Parser *parser)
{
    const char *variable;
    switch (parser->previous.tok_type) {
    case TK_NIL:
        return alloc_expr(parser, (Expr){.type = ExprNil, .as = {NULL}});
    case TK_NUMBER:
        return alloc_expr(parser,
                          (Expr){.type = ExprNumber,
                                 .as.number = parser->previous.info.number});
    case TK_TRUE:
        return alloc_expr(parser,
                          (Expr){.type = ExprBoolean, .as.boolean = true});
    case TK_FALSE:
        return alloc_expr(parser,
                          (Expr){.type = ExprBoolean, .as.boolean = false});
    case TK_NAME:
        variable = parser->previous.info.str;
        return alloc_expr(
            parser, (Expr){.type = ExprVariable, .as.variable = variable});
    case TK_STRING:
        variable = parser->previous.info.str;
        return alloc_expr(parser,
                          (Expr){.type = ExprString, .as.string = variable});
    default:
        return ASTNone;
    };
}

ASTIndex parse_binary(Parser *parser, ASTIndex left)
{
    ReservedToken op_type = parser->previous.tok_type;
    ASTIndex right = parse_expression(parser, PREC_UNARY);
    return alloc_expr(parser, (Expr){.type = ExprBinary,
                                     .as.binary.l_expr = left,
                                     .as.binary.r_expr = right,
                                     .as.binary.op = op_type});
}

ASTIndex parse_unary(Parser *parser)
{
    ReservedToken op_type = parser->previous.tok_type;
    Precedence precedence = rules[op_type].precedence;
    ASTIndex left = parse_expression(parser, (Precedence)(precedence + 1));
    return alloc_expr(parser, (Expr){.type = ExprUnary,
                                     .as.unary.l_expr = left,
                                     .as.unary.op = op_type});
}

ASTIndex parse_group(Parser *parser)
{
    ASTIndex group_expr = parse_expression(parser, PREC_ASSIGN);

    if (p_match(parser, TK_RPAREN)) {
        return alloc_expr(parser,
                          (Expr){.type = ExprGroup, .as.group = group_expr});
    }
    return ASTNone;
}

const ParserRule rules[] = {
    /* Constants  */
    [TK_NIL] = {parse_primary, NULL, PREC_NONE},
    [TK_NUMBER] = {parse_primary, NULL, PREC_NONE},
    [TK_STRING] = {parse_primary, NULL, PREC_NONE},
    [TK_NAME] = {parse_primary, NULL, PREC_NONE},
    [TK_TRUE] = {parse_primary, NULL, PREC_NONE},
    [TK_FALSE] = {parse_primary, NULL, PREC_NONE},

    /* arithmetic expressions */
    [TK_ADD] = {NULL, parse_binary, PREC_TERM},
    [TK_SUB] = {parse_unary, parse_binary, PREC_TERM},
    [TK_MUL] = {NULL, parse_binary, PREC_FACTOR},
    [TK_DIV] = {NULL, parse_binary, PREC_FACTOR},

    /* comparsion expressions */
    [TK_EQ_EQ] = {NULL, parse_binary, PREC_COMPARE},
    [TK_NEQ] = {NULL, parse_binary, PREC_COMPARE},
    [TK_LT] = {NULL, parse_binary, PREC_COMPARE},
    [TK_LT_EQ] = {NULL, parse_binary, PREC_COMPARE},
    [TK_GT] = {NULL, parse_binary, PREC_COMPARE},
    [TK_GT_EQ] = {NULL, parse_binary, PREC_COMPARE},

    [TK_LPAREN] = {parse_group, NULL, PREC_NONE},
    [TK_RPAREN] = {NULL, NULL, PREC_NONE},
    [TK_EOF] = {NULL, NULL, PREC_NONE},
};

ASTIndex parse_assignment_statement(Parser *parser)
{
    p_advance(parser);
    const char *name = parser->previous.info.str;
    ASTIndex value = ASTNone;
    if (p_match(parser, TK_EQ)) {
        value = parse_expression(parser, PREC_ASSIGN);
    }
    return alloc_stmt(parser, (Statement){.type = StmtAssignment,
                                          .as.assignment.name = name,
                                          .as.assignment.value = value});
}

ASTIndex parse_local_statement(Parser *parser)
{
    if (p_match(parser, TK_NAME)) {
        const char *name = parser->previous.info.str;
        ASTIndex value = ASTNone;
        if (p_match(parser, TK_EQ)) {
            value = parse_expression(parser, PREC_ASSIGN);
        }
        return alloc_stmt(parser, (Statement){.type = StmtLocalVar,
                                              .as.local_var.name = name,
                                              .as.local_var.value = value});
    } else {
        return ASTNone;
    }
}

ASTIndex parse_statement(Parser *parser)
{
    switch (parser->current.tok_type) {
    case TK_NAME:
        return parse_assignment_statement(parser);
    case TK_LOCAL:
        return parse_local_statement(parser);
    default:
        p_advance(parser);
        return ASTNone;
    };
}

ASTIndex parse_block(Parser *parser)
{
    ASTIndex first = ASTNone;
    ASTIndex last = ASTNone;
    while (!is_block_end(parser->current.tok_type) &&
           !(parser->current.tok_type == TK_ILLEGAL)) {
        ASTIndex stmt = parse_statement(parser);
        if (stmt == ASTNone) continue;
        if (first == ASTNone) {
            first = stmt;
        } else {
            parser->stmt_pool[last].next = stmt;
        }
        last = stmt;
    }

    return alloc_stmt(parser, (Statement){.type = StmtBlock,
                                          .as.block.first_stmt = first,
                                          .next = ASTNone});
}

Statement *get_statement(Parser *parser, ASTIndex stmt)
{
    return &parser->stmt_pool[stmt];
}

Expr *get_expression(Parser *parser, ASTIndex expr)
{
    return &parser->expr_pool[expr];
}

Parser *init_parser(const char *name)
{
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->lex_state = init_lexer(name);
    parser->expr_pool = NULL;
    parser->expr_len = 1;
    parser->expr_capacity = 0;
    parser->stmt_pool = NULL;
    parser->stmt_len = 1;
    parser->stmt_capacity = 0;
    parser->current = lconsume(parser->lex_state);
    return parser;
}

void destroy_parser(Parser *parser)
{
    free(parser->expr_pool);
    free(parser->stmt_pool);
    free(parser->lex_state);
    free(parser);
}
