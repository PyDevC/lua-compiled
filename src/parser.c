#include "parser.h"

#define is_block_end(x) ((x == TK_END) || (x == TK_ELSE) || (x == TK_EOF))

static ASTIndex alloc_expr(Parser *parser, Expr node)
{
    if (parser->expr_len >= parser->expr_capacity) {
        parser->expr_capacity =
            parser->expr_capacity == 0 ? 256 : 2 * parser->expr_capacity;
        parser->expr_pool =
            realloc(parser->expr_pool, parser->expr_capacity * sizeof(Expr));
    }
    parser->expr_pool[parser->expr_len] = node;
    return parser->expr_len++;
}

static ASTIndex alloc_stmt(Parser *parser, Statement stmt)
{
    if (parser->stmt_len >= parser->stmt_capacity) {
        parser->stmt_capacity =
            parser->stmt_capacity == 0 ? 256 : 2 * parser->stmt_capacity;
        parser->stmt_pool = realloc(parser->stmt_pool,
                                    parser->stmt_capacity * sizeof(Statement));
    }
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
    p_advance(parser);
    PrefixFn prefix_rule = rules[parser->current.tok_type].prefix;
    if (!prefix_rule) return ASTNone;

    ASTIndex left = prefix_rule(parser);
    while (precedence <= rules[parser->current.tok_type].precedence) {
        p_advance(parser);
        InfixFn infix_rule = rules[parser->current.tok_type].infix;
        left = infix_rule(parser, left);
    }
    return left;
}

ASTIndex parse_primary(Parser *parser)
{
    if (parser->previous.tok_type == TK_NIL) {
        return alloc_expr(parser, (Expr){.type = ExprNil, .as = {NULL}});
    }
    return ASTNone;
}

ASTIndex parse_binary(Parser *parser, ASTIndex left)
{
    ReservedToken op_type = parser->previous.tok_type;
    Precedence precedence = rules[op_type].precedence;
    ASTIndex right = parse_expression(parser, (Precedence)(precedence + 1));
    return alloc_expr(parser, (Expr){.type = ExprBinary,
                                     .as.binary.l_expr = left,
                                     .as.binary.r_expr = right,
                                     .as.binary.op = op_type});
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
    [TK_SUB] = {NULL, parse_binary, PREC_TERM},
    [TK_MUL] = {NULL, parse_binary, PREC_FACTOR},
    [TK_DIV] = {NULL, parse_binary, PREC_FACTOR},

    /* comparsion expressions */
    [TK_EQ_EQ] = {NULL, parse_binary, PREC_COMPARE},
    [TK_NEQ] = {NULL, parse_binary, PREC_COMPARE},
    [TK_LT] = {NULL, parse_binary, PREC_COMPARE},
    [TK_LT_EQ] = {NULL, parse_binary, PREC_COMPARE},
    [TK_GT] = {NULL, parse_binary, PREC_COMPARE},
    [TK_GT_EQ] = {NULL, parse_binary, PREC_COMPARE},
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
    p_advance(parser);
    if (p_match(parser, TK_NAME)) {
        const char *name = parser->previous.info.str;
        ASTIndex value = ASTNone;
        if (p_match(parser, TK_EQ)) {
            value = parse_expression(parser, PREC_ASSIGN);
        }
        return alloc_stmt(parser, (Statement){.type = StmtAssignment,
                                              .as.local_var.name = name,
                                              .as.local_var.value = value});
    } else {
        return ASTNone;
    }
}

ASTIndex parse_statement(Parser *parser)
{
    switch (parser->previous.tok_type) {
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
    while (!is_block_end(parser->previous.tok_type)) {
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

Parser *init_parser(const char *name)
{
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) return NULL;
    parser->lex_state = init_lexer(name);
    parser->expr_pool = NULL;
    parser->expr_len = 0;
    parser->expr_capacity = 0;
    parser->stmt_pool = NULL;
    parser->stmt_len = 0;
    parser->stmt_capacity = 0;
    parser->current = lpeek_next(parser->lex_state);
    p_advance(parser);
    return parser;
}

void destroy_parser(Parser *parser)
{
    free(parser->expr_pool);
    free(parser->stmt_pool);
    free(parser->lex_state);
    free(parser);
}
