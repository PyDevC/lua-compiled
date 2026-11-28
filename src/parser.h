#pragma once

#include "last.h"
#include "lexer.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,  // =
    PREC_OR,      // or
    PREC_AND,     // and
    PREC_COMPARE, // < > <= >= == ~=
    PREC_TERM,    // + -
    PREC_FACTOR,  // % * /
    PREC_CALL,    // func()
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

Parser *initParser(const char *name);
void destroyParser(Parser *parser);
void pAdvance(Parser *parser);
int pMatch(Parser *parser, ReservedToken tok_type);

AST_INDEX parseExpression(Parser *parser, Precedence prec);
AST_INDEX parseStatement(Parser *parser);
AST_INDEX parseBlock(Parser *parser);

Statement *getStatement(Parser *parser, AST_INDEX stmt);
Expr *getExpression(Parser *parser, AST_INDEX expr);

#ifdef PARSER_DEBUG
void printExpression(Parser *parser, Expr *expr);
void printStatement(Parser *parser, Statement *stmt);
#endif // PARSER_DEBUG
