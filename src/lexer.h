#pragma once

#include <stddef.h>
#include <stdio.h>

/* FileBuffer loads the entire file into the buffer with file info */
typedef struct
{
    FILE *f;
    const char *buffer;
    size_t fsize;    /* file length */
    size_t location; /* buffer index from where to read */
} FileBuffer;

/*
 * Reserved Symbols and keywords to be used while lexing, the order is not an
 * issue, but don't do any arithemetic with enum since we don't know if the
 * order will change or not.
 */
typedef enum {
    TK_ILLEGAL,
    TK_EOF,
    TK_EMPTY,
    TK_COMMENT,

    /*Keywords*/
    /*Range 4 - 25*/
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

    /*Symbols*/
    /*Range 26 - 46*/
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

    /*Types*/
    /*Range 47 - 49*/
    TK_NUMBER,
    TK_STRING,
    TK_NAME,
} ReservedToken;

typedef struct KeywordEntry
{
    const ReservedToken type;
    const char *name;
} KeywordEntry;

/*
 * Token Information if a number then the number value, and if a string then a
 * string, this supports every ReservedToken as it can be one of them,
 * char* for string is fine but we should add a new string library for string in
 * future.
 */
typedef union
{
    double number;
    const char *str;
} TokenInfo;

/* Token with type and token info */
typedef struct Token
{
    ReservedToken tok_type;
    TokenInfo info;
    size_t line;
    size_t column;
} Token;

/*
 * Lexer State for managing lexing per file scope.
 */
typedef struct LexerState
{
    FileBuffer *fbuffer;  /* look into file buffer*/
    Token tok;            /* current token */
    Token lookahead;      /* lookahead next token */
    const char *filename; /* file name for particular lexer state */
    size_t linenumber;    /* current line */
    size_t colnumber;     /* current column number */
    size_t lastline;      /* Last line for parsing token */
} LexerState;

LexerState *init_lexer(const char *filename);

/*
 * Return the lex_state->tok, and consume the lookahead if not consumed
 */
Token lconsume(LexerState *lex_state);

/*
 * Return the lookahead token and move forward in buffer
 */
Token lpeek_next(LexerState *lex_state);
const char *token_to_string(Token token);
