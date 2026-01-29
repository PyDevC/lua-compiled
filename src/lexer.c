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

/**
 * MainBuffer
 *
 * This struct consists of a buffer and two indexs which acts as a window for
 * locating token_buffer inside of main_buffer. Size of buffer is
 * READBUFFER_SIZE.
 *
 * If the buffer consists of NULL character then it is said to be end of stream.
 * We can use NULL character check to determine whether to further lookahead or
 * not.
 * When end == READBUFFER_SIZE we need to fill up the buffer, then set start
 * to 0 and end to 0 as new buffer is obtained.
 **/
static struct MainBuffer
{
    char buffer[READBUFFER_SIZE]; /* Content of buffer */
    int start; /* index of the location where the token_buffer starts */
    int end;   /* index of the location where the token_buffer ends */
} main_buffer;

/**
 * TokenBuffer
 *
 * This struct is a smaller version of MainBuffer but with lineno and colno.
 * Functions can only access token_buffer. There is no direct mainipulation of
 * MainBuffer.
 *
 * This is a cyclic kind of buffer start and end represent the start and end of
 * stream local to token_buffer. This buffer is checked for NULL character, if
 * it consists of NULL then it means we have EOF
 **/
static struct TokenBuffer
{
    char buffer[TOKENBUFFER_SIZE]; /* Buffer where the lexer looks for token */
    long int lineno; /* Current line number which you are parsing */
    int start;       /* start of token buffer */
    int end;         /* end of a token buffer */
    short colno;     /* Current column number you are parsing */
} token_buffer;

static int lexeme_begin; /* Gets updated after previous token is captured */
static int forward;      /* Gets update with every character */

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
char get_next_char();
char peek_next_char();
void skip_whitespaces();
void skip_comments();

TokenStruct make_token(TokenType type);
TokenType read_keyword(const char *identifier_literal);

TokenStruct read_identifier();
TokenStruct read_number();
TokenStruct scantoken_symbol(char c);
TokenStruct get_next_token();

/* Function Definitions */

/* Operations on MainBuffer */
void main_buffer_fill_buffer()
{
    size_t read_counter = fread(&main_buffer.buffer, 1, READBUFFER_SIZE, f);

    /* Checking EOF */
    if (read_counter != READBUFFER_SIZE) {
        if (feof(f)) {
            main_buffer.buffer[(int)read_counter] = '\0'; /* End of Stream */
        } else if (ferror(f)) {
            E(fprintf(stderr, "Error while reading file\n"));
        }
    }
}

void main_buffer_init()
{
    main_buffer.start = 0;
    main_buffer.end = 0;
    main_buffer_fill_buffer();
}

/* Operations on TokenBuffer */
void token_buffer_fill_buffer()
{
    while (1) {
        char c = main_buffer.buffer[main_buffer.end];
        if (c == '\0') {
            token_buffer.buffer[token_buffer.end] = '\0';
            break;
        }

        int next_token_end = (token_buffer.end + 1) % TOKENBUFFER_SIZE;
        if (next_token_end == token_buffer.start) {
            break;
        }

        token_buffer.buffer[token_buffer.end] = c;

        main_buffer.end++;
        if (main_buffer.end == READBUFFER_SIZE) {
            main_buffer.end = 0;
            main_buffer_fill_buffer();
        }

        token_buffer.end++;
        if (token_buffer.end == TOKENBUFFER_SIZE) {
            token_buffer.end = 0;
        }
    }
}

void token_buffer_init()
{
    token_buffer.colno = 0;
    token_buffer.lineno = 1;
    token_buffer.start = 0;

    main_buffer.start = 0;
    main_buffer.end = token_buffer.end;

    token_buffer_fill_buffer();
}

char get_next_char()
{
    char c = token_buffer.buffer[forward];
    if (c == '\0') {
        return EOF;
    }
    forward++;
    if (forward == TOKENBUFFER_SIZE) {
        /* If the forward reaches the end redirect it to the begining */
        forward = 0;
    }
    return c;
}

char peek_next_char() { return token_buffer.buffer[forward]; }

void skip_whitespaces()
{
    while (is_blank(peek_next_char())) {
        get_next_char();
    }
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
        return (TokenStruct){.type = type, .literal = NULL};
    }

    size_t length;
    if (forward >= lexeme_begin) {
        length = (size_t)(forward - lexeme_begin);
    } else {
        length = (size_t)(TOKENBUFFER_SIZE - lexeme_begin + forward);
    }

    char *literal = malloc(length + 1);
    if (!literal) {
        E(fprintf(stderr, "Out of memory\n"));
        exit(1);
    }

    int temp_begin = lexeme_begin;
    for (size_t i = 0; i < length; i++) {
        literal[i] = token_buffer.buffer[temp_begin];
        temp_begin = (temp_begin + 1) % TOKENBUFFER_SIZE;
    }
    literal[length] = '\0';
    D(fprintf(stdout, "DEBUG: src/lexer.c/make_token: literal -> '%s'\n",
              literal));

    lexeme_begin = forward;
    token_buffer.start = forward;
    token_buffer_fill_buffer();

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
                  token_buffer.buffer[forward]));

        get_next_char();
    }

    D(fprintf(stdout,
              "DEBUG: src/lexer.c/read_identifier: lexeme_begin-> '%c', "
              "forward-> '%c'\n",
              token_buffer.buffer[lexeme_begin], token_buffer.buffer[forward]));

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
    case '<':
        if (peek_next_char() == '=') {
            get_next_char();
            token = make_token(LESS_T_EQUAL);
            break;
        }
        token = make_token(LESS_T);
        break;
    case '>':
        if (peek_next_char() == '=') {
            get_next_char();
            token = make_token(GREATER_T_EQUAL);
            break;
        }
        token = make_token(GREATER_T_EQUAL);
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
    // skip_comments();
    // skip_whitespaces();
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
    main_buffer_init();
    token_buffer_init();
    lexeme_begin = 0;
    forward = 0;
    return 0;
}
