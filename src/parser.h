#pragma once

#include "allocator.h"
#include "last.h"
#include "lexer.h"

#define u32 uint32_t

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,  // =
    PREC_OR,      // or
    PREC_AND,     // and
    PREC_COMPARE, // < > <= >= == ~=
    PREC_TERM,    // + -
    PREC_FACTOR,  // % * /
    PREC_UNARY,   // not #
    PREC_POWER,   // ^
    PREC_PRIMARY
} Precedence;

typedef struct Parser
{
    Token current;
    Token previous;
    LexerState *lex_state;
    Expr *expr_pool;
    Statement *stmt_pool;
    u32 expr_capacity;
    u32 expr_len;
    u32 stmt_capacity;
    u32 stmt_len;
} Parser;

Parser *init_parser(const char *name);
void destory_parser(Parser *parser);
void p_advance(Parser *parser);
int p_match(Parser *parser, ReservedToken tok_type);

ASTIndex parse_expression(Parser *parser, Precedence prec);
ASTIndex parse_statement(Parser *parser);
ASTIndex parse_block(Parser *parser);

Statement *get_statement(Parser *parser, ASTIndex stmt);
Expr *get_expression(Parser *parser, ASTIndex expr);
