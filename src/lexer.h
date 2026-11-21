/* ========================================================================

   $File: lexer.h
   $Date: 2026/11/20
   $Revision: 1

   ReservedToken / Token / KeywordEntry keep their original names so the
   grammar notes in Grammar/ still line up with the code. Only the state
   object changes: the file is already in memory and the cursor walks it.

   ======================================================================== */

#ifndef LUA_LEXER_H
#define LUA_LEXER_H

#include "defs.h"

typedef enum {
    TK_ILLEGAL,
    TK_EOF,
    TK_EMPTY,
    TK_COMMENT,

    /* Keywords: 4 - 25 */
    TK_AND,
    TK_BREAK,
    TK_DO,
    TK_ELSE,
    TK_ELSEIF,
    TK_END,
    TK_FALSE,
    TK_FOR,
    TK_FUNCTION,
    TK_IF,
    TK_IN,
    TK_LOCAL,
    TK_NIL,
    TK_NOT,
    TK_OR,
    TK_REPEAT,
    TK_RETURN,
    TK_THEN,
    TK_TRUE,
    TK_UNTIL,
    TK_WHILE,

    /* Symbols: 26 - 47 */
    TK_COMMA,
    TK_HASH,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_SEMICOLON,
    TK_EQ,
    TK_EQ_EQ,
    TK_NEQ,
    TK_BANG,
    TK_GT,
    TK_GT_EQ,
    TK_LT,
    TK_LT_EQ,
    TK_ADD,
    TK_SUB,
    TK_MUL,
    TK_DIV,

    /* Types: 48 - 50 */
    TK_NUMBER,
    TK_STRING,
    TK_NAME,

    TK_COUNT,
} ReservedToken;

typedef struct KeywordEntry
{
    const ReservedToken type;
    const char *name;
} KeywordEntry;

typedef union
{
    f64 num;
    const char *str;
} TokenInfo;

typedef struct Token
{
    ReservedToken tok_type;
    TokenInfo info;
    u32 line;
    u32 column;
} Token;

/* The cursor owns no memory. Base/End bound the file, At walks it. */
typedef struct lexer_cursor
{
    char *Base;
    char *At;
    char *End;
    u32 Line;
    u32 Column;
    memory_arena *Arena;
} lexer_cursor;

typedef struct LexState
{
    lexer_cursor Cursor;
    Token Curr;
    Token Lookahead;
    b32 HasLookahead;
} LexState;

void InitLexer(LexState *LexState, memory_arena *Arena, source_file *Source);
Token ScanNextToken(lexer_cursor *Cursor);
Token lconsume(LexState *LexState);
Token lpeek_next(LexState *LexState);
const char *token_to_string(Token Tok);

#endif /* LUA_LEXER_H */
