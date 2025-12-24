#ifndef LEXER_H
#define LEXER_H

#define READBUFFER_SIZE 16 // 16 bytes
#define TOTALREADBUFFER_SIZE 2 * READBUFFER_SIZE

#include <stdio.h>

extern FILE *f;

typedef enum {
    ILLEGAL,
    /* 0 */ /* ILLEGAL should be first */
    _EOF,
    COMMA,
    HASH,
    LPAREN,
    /* 4 */ /* Control Flow symbols */
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    EQUAL,
    /* 12 */ /* Comparision Symbols */
    EQUAL_EQUAL,
    NOT_EQUAL,
    BANG,
    AND,
    /* 16 */ /* 22 Keywords */
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
    ADD,
    /* 38 */ /* Arithmetic Symbols */
    SUB,
    MUL,
    DIV,
    LITERAL_NUMBER,
    /* 42 */ /* Misc */
    LITERAL_STRING,
    IDENTIFIER,
} TokenType;

typedef struct TokenStruct {
    TokenType type;
    char *literal;
    size_t linenumber;
    size_t colnumber;
} TokenStruct;

typedef struct {
    TokenType type;
    const char *literal;
} KeywordEntry;

int init_lexer(const char *filename);
// returns the exit of lexer 0 means everything is
// alright but 1 means that something went wrong

TokenStruct get_next_token();

#endif // LEXER_H
