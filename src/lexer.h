#ifndef LEXER_H
#define LEXER_H

#define READBUFFER_SIZE 16 // 16 bytes
#define TOTALREADBUFFER_SIZE 2 * READBUFFER_SIZE

#include <stdio.h>

extern FILE *f;

typedef enum {
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACKET,
  RBRACKET,
  SEMICOLON,
  EQUAL,
  EQUAL_EQUAL,
  NOT_EQUAL,
  BANG,
  COMMA,
  HASH,
  LITERAL_NUMBER,
  LITERAL_STRING,
  IDENTIFIER,
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
  ADD,
  SUB,
  MUL,
  DIV,
  _EOF,
  ILLEGAL
} TokenType;

typedef struct TokenStruct {
  char *literal;
  size_t linenumber;
  size_t colnumber;
  TokenType type;
} TokenStruct;

typedef struct {
  const char *literal;
  TokenType type;
} KeywordEntry;

int init_lexer(const char *filename);
// returns the exit of lexer 0 means everything is
// alright but 1 means that something went wrong

TokenStruct get_next_token();

#ifdef DEBUG_LUA

char get_next_char();
int peek_next_char();
void skip_whitespaces();
void skip_comments();
TokenStruct make_token(TokenType);
TokenType read_keyword(const char *);
TokenStruct read_identifier();
TokenStruct read_number();
TokenStruct scantoken_symbol(char);
TokenStruct get_next_token();

#endif // DEBUG_LUA

#endif // LEXER_H
