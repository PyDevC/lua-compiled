/* ========================================================================

   $File: parser.h
   $Date: 2026/11/20
   $Revision: 1

   A Pratt (top-down operator precedence) parser. The rule table is indexed
   by ReservedToken, the same way the sim86 decoder in Computer Enhance
   indexes its instruction formats: no recursive precedence functions, just
   data saying "this token has a prefix parser, an infix parser, and a
   binding power".

   ======================================================================== */

#ifndef LUA_PARSER_H
#define LUA_PARSER_H

#include "last.h"
#include "lexer.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGN,  /* = (not an expression operator) */
    PREC_OR,      /* or */
    PREC_AND,     /* and */
    PREC_COMPARE, /* < > <= >= == ~= */
    PREC_TERM,    /* + - */
    PREC_FACTOR,  /* * / */
    PREC_UNARY,   /* not - # */
    PREC_CALL,    /* f() */
    PREC_PRIMARY,
} Precedence;

typedef struct Parser
{
    Token Curr;
    LexState LexState;
    source_file *Source;
    memory_arena *Arena;
    message_log *Log;
    u32 StartErrorCount;

    Expr *ExprPool;
    u32 ExprCap;
    u32 ExprLen;

    Statement *StmtPool;
    u32 StmtCap;
    u32 StmtLen;

} Parser;

void InitParser(Parser *Parser, memory_arena *Arena, source_file *Source,
                message_log *Log);
ASTIndex ParseChunk(Parser *Parser);
b32 ParserOK(Parser *Parser);

Expr *GetExpr(Parser *Parser, ASTIndex Index);
Statement *GetStatement(Parser *Parser, ASTIndex Index);

void DumpExpr(Parser *Parser, ASTIndex Index);
void DumpStatement(Parser *Parser, ASTIndex Index);

#endif /* LUA_PARSER_H */
