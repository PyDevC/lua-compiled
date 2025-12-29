#include "lexer.h"
#include "errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TokenStruct peeked_token = {};
static short has_peeked = 0;

#define is_alpha(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define is_digit(c) (c >= '0' && c <= '9')
#define is_alnum(c) (is_digit(c) || is_alpha(c))
#define is_blank(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r')
#define is_identifier_start(c) (is_alpha(c) || c == '_')
#define is_identifier(c) (is_alnum(c) || c == '_')

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

int fill_buffer(int bufno)
{
    int idx = bufno * READBUFFER_SIZE;
    size_t read_counter = fread(&buffer[idx], 1, READBUFFER_SIZE, f);

    buffer[idx + (int)read_counter] = '\0';

    return (read_counter > 0);
}

char get_next_char()
{
    char c = *forward;

    if (c == '\0') {
        /* Here bufno shows what part of buffer we are at */
        int bufno = (forward >= &buffer[TOTALREADBUFFER_SIZE]) ? 1 : 0;
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

char peek_next_char()
{
    char c = *forward;
    if (c == '\0') {
        c = get_next_char();
        return c;
    }
    return *forward;
}

void skip_whitespaces()
{
    char c = get_next_char();
    if (c == EOF) {
        *forward = EOF;
        return;
    }
    while (is_blank(c)) {
        c = get_next_char();
    }
    forward--;
}

void skip_comments()
{
    // only single line comments supported
    char c = get_next_char();
    if (c == EOF) {
        return;
    }
    if (c == '-') {
        if (peek_next_char() == '-') {
            c = get_next_char();
            while (1) {
                if (c == '\n') {
                    skip_comments();
                    return;
                }
                D(fprintf(stdout,
                          "DEBUG: src/lexer.c/skip_comments: c -> '%c'\n", c));
                c = get_next_char();
            }
        } else {
            forward--;
        }
    }
    forward--;
}

TokenStruct make_token(TokenType type)
{
    if (type == _EOF) {
        return (TokenStruct){.type = type};
    }

    long int length = forward - lexeme_begin;
    char *literal = (char *)malloc((size_t)length);
    strncpy(literal, lexeme_begin, (size_t)length);
    literal[length] = '\0';
    lexeme_begin = forward;

    D(fprintf(stdout, "DEBUG: src/lexer.c/make_token: literal -> '%s'\n",
              literal));

    return (TokenStruct){.type = type, .literal = literal};
}

TokenType read_keyword(const char *identifier_literal)
{
    for (int i = 0; keywords[i].literal != NULL; i++) {
        if (strcmp(keywords[i].literal, identifier_literal) == 0) {
            return keywords[i].type;
        }
    }
    return IDENTIFIER;
}

TokenStruct read_identifier()
{
    while (is_identifier(peek_next_char())) {

        D(fprintf(stdout,
                  "DEBUG: src/lexer.c/read_identifier: forward-> '%c'\n",
                  *forward));

        get_next_char();
    }

    D(fprintf(stdout,
              "DEBUG: src/lexer.c/read_identifier: lexeme_begin-> '%c', "
              "forward-> '%c'\n",
              *lexeme_begin, *forward));

    TokenStruct token = make_token(IDENTIFIER);
    token.type = read_keyword(token.literal);
    return token;
}

TokenStruct read_number()
{
    int is_floating = 0; /* 1 if number is floating */
    while (is_digit(peek_next_char()) || peek_next_char() == '.') {

        if (peek_next_char() == '.') {
            if (is_floating) {
                E(fprintf(stderr, "Syntax Error: Expected Number but got '%c'",
                          peek_next_char()));
            } else {
                is_floating = 1;
            }
        }

        get_next_char();
    }
    return make_token(LITERAL_NUMBER);
}

TokenStruct scantoken_symbol(char c)
{
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
        if (peek_next_char() == '=') {
            get_next_char();
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
        if (peek_next_char() == '=') {
            get_next_char();
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

TokenStruct get_next_token()
{
    TokenStruct token = {0}; /* First time the TokenType should be illegal */
    skip_whitespaces();
    skip_comments();
    skip_whitespaces();
    lexeme_begin = forward; /* Marking Start of Token */

    char c = get_next_char();
    D(fprintf(stdout, "DEBUG: src/lexer.c/get_next_token: c -> '%c'\n", c));

    if (c == EOF) {
        token = make_token(_EOF);
        return token;
    }

    if (is_identifier_start(c)) {
        token = read_identifier();
    } else if (is_digit(c)) {
        token = read_number();
    } else {
        token = scantoken_symbol(c);
    }
    return token;
}

TokenStruct peek_next_token()
{
    if (!has_peeked) {
        peeked_token = get_next_token();
        has_peeked = 1;
        return peeked_token;
    }
    return peeked_token;
}
TokenStruct consume_token()
{
    if (has_peeked) {
        has_peeked = 0;
        return peeked_token;
    }
    /* Print the Token in Debug mode */
    D(TokenStruct token = get_next_token());
    D(fprintf(stdout, "Consumed Token -> %s, %d\n", token.literal, token.type));
    D(return token);
    return get_next_token();
}

int init_lexer(const char *filename)
{
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
