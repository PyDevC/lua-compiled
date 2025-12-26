#ifndef LEXER_H
#define LEXER_H

#define READBUFFER_SIZE 16 // 16 bytes
#define TOTALREADBUFFER_SIZE 2 * READBUFFER_SIZE

#include <stdio.h>

extern FILE *f;

typedef enum {
    /* 0 */ /* ILLEGAL should be first */
    ILLEGAL,
    _EOF,
    COMMA,
    HASH,
    /* 4 */ /* Control Flow symbols */
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    /* 12 */ /* Comparision Symbols */
    EQUAL,
    EQUAL_EQUAL,
    NOT_EQUAL,
    BANG,
    /* 16 */ /* 22 Keywords */
    AND,
    BREAK,
    DO,
    ELSE,
    ELSEIF,
    END,
    FALSE,
    FOR,
    FUNCTION,
    MAIN,
    IF,
    IN,
    LOCAL,
    NIL,
    NOT,
    OR,
    REPEAT,
    RETURN,
    THEN,
    TRUE,
    UNTIL,
    WHILE,
    /* 38 */ /* Arithmetic Symbols */
    ADD,
    SUB,
    MUL,
    DIV,
    /* 42 */ /* Misc */
    LITERAL_NUMBER,
    LITERAL_STRING,
    IDENTIFIER,
} TokenType;

typedef struct TokenStruct
{
    TokenType type;
    char *literal;
    size_t linenumber;
    size_t colnumber;
} TokenStruct;

typedef struct
{
    TokenType type;
    const char *literal;
} KeywordEntry;

int init_lexer(const char *filename);
// returns the exit of lexer 0 means everything is
// alright but 1 means that something went wrong

TokenStruct consume_token();
TokenStruct peek_next_token();

#endif // LEXER_H
