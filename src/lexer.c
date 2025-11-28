#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *f = NULL;

char buffer[TOTALREADBUFFER_SIZE + 1]; /* Buffer window to store blocks of source code */
char *lexeme_begin;
char *forward;

/* Keywords with their token name, tokens defines in lexer.h */
static const KeywordEntry keywords[] = {
    {"and", AND},       {"break", BREAK},   {"do", DO},
    {"else", ELSE},     {"elseif", ELSEIF}, {"end", END},
    {"false", FALSE},   {"for", FOR},       {"function", FUNCTION},
    {"if", IF},         {"in", IN},         {"local", LOCAL},
    {"nil", NIL},       {"not", NOT},       {"or", OR},
    {"repeat", REPEAT}, {"return", RETURN}, {"then", THEN},
    {"true", TRUE},     {"until", UNTIL},   {"while", WHILE},
    {NULL, ILLEGAL},    {"main", MAIN}};

int fill_buffer(int bufno) {
  int idx = bufno * READBUFFER_SIZE;
  size_t read_counter = fread(&buffer[idx], 1, READBUFFER_SIZE, f);
  buffer[idx + (int)read_counter] = '\0';
  if (read_counter < READBUFFER_SIZE) {
    return 0;
  }
  return 1;
}

char get_next_char() {
  char c = *forward;
  if (c == '\0') {
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
  return c;
}

int peek_next_char() { return *forward; }

void skip_whitespaces() {
  char c;
  do {
    c = get_next_char();
  } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
  forward--;
  lexeme_begin = forward;
}

void skip_comments() {
  char c;
  // for single line: lets not support multiline comment yet
  c = get_next_char();
  if (c == '-') {
    if (peek_next_char() == '-') {
      while (c != '\n') {
        c = get_next_char();
      }
      forward--;
      lexeme_begin = forward;
      return;
    }
  }
  forward--;
  lexeme_begin = forward;
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
  while (isalnum(peek_next_char()) || peek_next_char() == '_') {
    if (peek_next_char() == ' ' || peek_next_char() == '\n') {
      break;
    }
    get_next_char();
  }
  TokenStruct token = make_token(IDENTIFIER);
  token.type = read_keyword(token.literal);
  return token;
}

TokenStruct read_number() {
  while (isdigit(peek_next_char()) || peek_next_char() == '.') {
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
  skip_comments(); // this should always be above skip_whitespaces
  skip_whitespaces();
  char c = get_next_char();
  if (c == EOF) {
    token = make_token(_EOF);
    return token;
  }
  if (isalpha(c) || c == '_') {
    return read_identifier();
  } else if (isdigit(c)) {
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
  fill_buffer(0);
  fill_buffer(1);
  return 0;
}
