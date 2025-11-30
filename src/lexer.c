#include "lexer.h"
#include "errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define is_identifier_start(c)                                                 \
  ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')

#define is_identifier_char(c)                                                  \
  ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||                         \
   (c >= '0' && c <= '9') || c == '_')

#define is_number_char(c) ((c >= '0' && c <= '9') || (c == '.'))

FILE *f = NULL;

/* Buffer window to store blocks of source code */
char buffer[TOTALREADBUFFER_SIZE + 1];
char *lexeme_begin; /* Gets updated after previous token is captured */
char *forward;      /* Gets update with every character */
int linenumber;     /*Keeping this global for now but there might be better way
                          to do this*/
int colnumber;      /*Same for this*/

/* Keywords with their token name, tokens defines in lexer.h */
static const KeywordEntry keywords[] = {
    {AND, "and"},       {BREAK, "break"},   {DO, "do"},
    {ELSE, "else"},     {ELSEIF, "elseif"}, {END, "end"},
    {FALSE, "false"},   {FOR, "for"},       {FUNCTION, "function"},
    {IF, "if"},         {IN, "in"},         {LOCAL, "local"},
    {NIL, "nil"},       {NOT, "not"},       {OR, "or"},
    {REPEAT, "repeat"}, {RETURN, "return"}, {THEN, "then"},
    {TRUE, "true"},     {UNTIL, "until"},   {WHILE, "while"},
    {ILLEGAL, NULL}};

/* Function Declarations */
int fill_buffer(int bufno);

char get_next_char();
char peek_next_char();
void skip_whitespaces();

TokenStruct make_token(TokenType type);
TokenType read_keyword(const char *identifier_literal);

TokenStruct read_identifier();
TokenStruct read_number();
TokenStruct scantoken_symbol(char c);
TokenStruct get_next_token();

/* Function Definitions */

int fill_buffer(int bufno) {
  int idx = bufno * READBUFFER_SIZE;
  size_t read_counter = fread(&buffer[idx], 1, READBUFFER_SIZE, f);

  buffer[idx + (int)read_counter] = '\0';
  /*Ending should be NULL character to determine whether we have
             reached end of buffer or not*/

  if (read_counter < READBUFFER_SIZE) {
    return 0;
  }

  return 1;
}

char get_next_char() {
  char c = *forward;

  if (c == '\0') {
    /* Here bufno shows what part of buffer we are at */
    int bufno = (forward >= &buffer[READBUFFER_SIZE]) ? 1 : 0;
    if (bufno == 0) {
      forward = &buffer[READBUFFER_SIZE];
    } else {
      forward = &buffer[0];
    }
    int success = fill_buffer(1 - bufno);
    if (!success) {
      return EOF;
    }
    c = *forward;
  }
  forward++;
  colnumber++;
  if (c == '\n') {
    linenumber++;
    colnumber = 0;
  }

  return c;
}

char peek_next_char() { return *forward; }

void skip_whitespaces() {
  char c = get_next_char();
  while (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
    c = get_next_char();
  }
  forward--;
}

TokenStruct make_token(TokenType type) {
  long int length = forward - lexeme_begin;
  char *literal = (char *)malloc((size_t)length);
  strncpy(literal, lexeme_begin, (size_t)length);
  literal[length] = '\0';
  lexeme_begin = forward;
  return (TokenStruct){.type = type, .literal = literal};
}

TokenType read_keyword(const char *identifier_literal) {
  for (int i = 0; keywords[i].literal != NULL; i++) {
    if (strcmp(keywords[i].literal, identifier_literal) == 0) {
      return keywords[i].type;
    }
  }
  return IDENTIFIER;
}

TokenStruct read_identifier() {
  while (is_identifier_char(peek_next_char())) {
    get_next_char();
  }
  TokenStruct token = make_token(IDENTIFIER);
  token.type = read_keyword(token.literal);
  return token;
}

TokenStruct read_number() {
  int is_floating = 0; /* 1 if number is floating */
  while (is_number_char(peek_next_char())) {
    if (peek_next_char() == '.' && is_floating) {
      E(fprintf(stderr, "Syntax Error: Expected Number but got %c",
                peek_next_char()));
    }
    get_next_char();
  }
  TokenStruct token = make_token(LITERAL_NUMBER);
  return token;
}

TokenStruct scantoken_symbol(char c) {
  TokenStruct token;
  switch (c) {
  case '(':
    token = make_token(LPAREN);
    break;
  case ')':
    token = make_token(RPAREN);
    break;
  case '{':
    token = make_token(LBRACE);
    break;
  case '}':
    token = make_token(RBRACE);
    break;
  case '[':
    token = make_token(LBRACKET);
    break;
  case ']':
    token = make_token(RBRACKET);
    break;
  case ';':
    token = make_token(SEMICOLON);
    break;
  case ',':
    token = make_token(COMMA);
    break;
  case '~':
    if (get_next_char() == '=') {
      token = make_token(NOT_EQUAL);
      break;
    }
    token = make_token(ILLEGAL);
    break;
  case '!':
    token = make_token(BANG);
    break;
  case '#':
    token = make_token(HASH);
    break;
  case '+':
    token = make_token(ADD);
    break;
  case '-':
    token = make_token(SUB);
    break;
  case '*':
    token = make_token(MUL);
    break;
  case '/':
    token = make_token(DIV);
    break;
  case '=':
    if (get_next_char() == '=') {
      token = make_token(EQUAL_EQUAL);
      break;
    }
    token = make_token(EQUAL);
    break;
  default:
    token = make_token(ILLEGAL);
    break;
  };
  return token;
}

TokenStruct get_next_token() {
  TokenStruct token = {0};
  skip_whitespaces();
  lexeme_begin = forward;
  char c = get_next_char();
  if (c == EOF) {
    token = make_token(_EOF);
    return token;
  }
  if (is_identifier_start(c)) {
    return read_identifier();
  } else if (is_number_char(c)) {
    return read_number();
  } else {
    token = scantoken_symbol(c);
  }
  return token;
}

int init_lexer(const char *filename) {
  f = fopen(filename, "r");
  if (!f) {
    return 1;
  }
  lexeme_begin = buffer;
  forward = buffer;
  linenumber = 1;
  colnumber = 0;
  fill_buffer(0);
  fill_buffer(1);
  return 0;
}
