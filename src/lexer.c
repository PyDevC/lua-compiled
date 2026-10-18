#include "lexer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define is_alpha(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define is_digit(c) (c >= '0' && c <= '9')
#define is_alnum(c) (is_digit(c) || is_alpha(c))
#define is_blank(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r')
#define is_identifier_start(c) (is_alpha(c) || c == '_')
#define is_identifier(c) (is_alnum(c) || c == '_')

static const KeywordEntry keywords[] = {
    {TK_AND, "and"},       {TK_BREAK, "break"},   {TK_DO, "do"},
    {TK_ELSE, "else"},     {TK_ELSEIF, "elseif"}, {TK_END, "end"},
    {TK_FALSE, "false"},   {TK_FOR, "for"},       {TK_FUNCTION, "function"},
    {TK_IF, "if"},         {TK_IN, "in"},         {TK_LOCAL, "local"},
    {TK_NIL, "nil"},       {TK_NOT, "not"},       {TK_OR, "or"},
    {TK_REPEAT, "repeat"}, {TK_RETURN, "return"}, {TK_THEN, "then"},
    {TK_TRUE, "true"},     {TK_UNTIL, "until"},   {TK_WHILE, "while"}};

static ReservedToken keyword_type(const char *name)
{
    size_t num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < num_keywords; i++) {
        if (strcmp(keywords[i].name, name) == 0) {
            return keywords[i].type;
        }
    }
    return TK_EMPTY;
}

static const char *keyword_name(Token token)
{
    return keywords[token.tok_type].name;
}

const char *token_to_string(Token tok)
{
    switch (tok.tok_type) {
    case TK_EOF:
    case TK_EMPTY:
        return "";

    case TK_ILLEGAL:
    case TK_COMMENT:
    case TK_STRING:
    case TK_NAME:
        return tok.info.str ? tok.info.str : "";

    case TK_NUMBER:
        return "[NUMBER]";
    case TK_AND:
    case TK_BREAK:
    case TK_DO:
    case TK_ELSE:
    case TK_ELSEIF:
    case TK_END:
    case TK_FALSE:
    case TK_FOR:
    case TK_FUNCTION:
    case TK_IF:
    case TK_IN:
    case TK_LOCAL:
    case TK_NIL:
    case TK_NOT:
    case TK_OR:
    case TK_REPEAT:
    case TK_RETURN:
    case TK_THEN:
    case TK_TRUE:
    case TK_UNTIL:
    case TK_WHILE:
        return keyword_name(tok);

    case TK_COMMA:
        return ",";
    case TK_HASH:
        return "#";
    case TK_LPAREN:
        return "(";
    case TK_RPAREN:
        return ")";
    case TK_LBRACE:
        return "{";
    case TK_RBRACE:
        return "}";
    case TK_LBRACKET:
        return "[";
    case TK_RBRACKET:
        return "]";
    case TK_SEMICOLON:
        return ";";
    case TK_EQ:
        return "=";
    case TK_EQ_EQ:
        return "==";
    case TK_NEQ:
        return "~=";
    case TK_BANG:
        return "!";
    case TK_GT:
        return ">";
    case TK_GT_EQ:
        return ">=";
    case TK_LT:
        return "<";
    case TK_LT_EQ:
        return "<=";
    case TK_ADD:
        return "+";
    case TK_SUB:
        return "-";
    case TK_MUL:
        return "*";
    case TK_DIV:
        return "/";
    };

    return "";
}

static char peek_next_char(LexerState *lex_state)
{
    size_t location = lex_state->fbuffer->location;
    if (location >= lex_state->fbuffer->fsize) {
        return EOF;
    }
    return lex_state->fbuffer->buffer[location];
}

static char get_next_char(LexerState *lex_state)
{
    char c = peek_next_char(lex_state);
    if (c == EOF) {
        return EOF;
    }
    lex_state->fbuffer->location++;
    if (c == '\n') {
        lex_state->linenumber++;
        lex_state->colnumber = 0;
    } else {
        lex_state->colnumber++;
    }
    return c;
}

static void skip_whitespace(LexerState *lex_state)
{
    while (is_blank(peek_next_char(lex_state))) {
        get_next_char(lex_state);
    }
}

Token make_identifier_token(Token token, LexerState *lex_state)
{
    // TODO: set max identifier name limit
    // right now the hard limit is 63 chars
    char lname[64];
    int idx = 0;
    while (idx < 63 && is_identifier(peek_next_char(lex_state))) {
        lname[idx] = get_next_char(lex_state);
        idx++;
    }
    lname[idx] = '\0'; // mark end of string
    ReservedToken kw = keyword_type(lname);
    if (kw != TK_EMPTY) {
        token.tok_type = kw;
        return token;
    }

    token.info.str = strdup(lname);
    token.tok_type = TK_NAME;
    return token;
}

Token make_number_token(Token token, LexerState *lex_state)
{
    char lname[64] = "";
    int idx = 0;
    while (is_digit(peek_next_char(lex_state))) {
        lname[idx] = get_next_char(lex_state);
        idx++;
    }
    lname[idx] = '\0'; // mark end of string
    token.info.number = atof(lname);
    token.tok_type = TK_NUMBER;
    return token;
}

Token make_string_token(Token token, LexerState *lex_state)
{
    // set hard limit for string length
    char lname[200] = {'"'};
    int idx = 0;
    get_next_char(lex_state);
    while (peek_next_char(lex_state) != '"') {
        lname[idx] = get_next_char(lex_state);
        idx++;
    }
    lname[idx++] = '"';
    lname[idx] = '\0';
    token.info.str = strdup(lname);
    if (token.info.str == NULL) {
        token.tok_type = TK_ILLEGAL;
    } else {
        token.tok_type = TK_STRING;
    }
    return token;
}

Token make_comment_token(Token token, LexerState *lex_state)
{
    char lname[200] = {'-', '-'};
    int idx = 0;
    get_next_char(lex_state);
    while (peek_next_char(lex_state) != '\n') {
        lname[idx] = get_next_char(lex_state);
        idx++;
    }
    lname[idx] = '\0';
    token.info.str = strdup(lname);
    token.tok_type = TK_COMMENT;
    return token;
}

// This will actually give you the token
Token lnext_token(LexerState *lex_state)
{
    Token token = {};
    token.tok_type = TK_ILLEGAL;
    token.info.str = NULL;

    skip_whitespace(lex_state);

    token.line = lex_state->linenumber;
    token.column = lex_state->colnumber;

    if (peek_next_char(lex_state) == EOF) {
        token.tok_type = TK_EOF;
        return token;
    }

    char c = peek_next_char(lex_state);
    if (is_identifier(c)) {
        if (is_identifier_start(c)) {
            return make_identifier_token(token, lex_state);
        } else if (is_digit(c)) {
            return make_number_token(token, lex_state);
        }
    }

    // clang-format off
    switch (peek_next_char(lex_state)) {
        case ',': { token.tok_type = TK_COMMA; } break;
        case '#': { token.tok_type = TK_HASH; } break;
        case '(': { token.tok_type = TK_LPAREN; } break;
        case ')': { token.tok_type = TK_RPAREN; } break;
        case '{': { token.tok_type = TK_LBRACE; } break;
        case '}': { token.tok_type = TK_RBRACE; } break;
        case '[': { token.tok_type = TK_LBRACKET; } break;
        case ']': { token.tok_type = TK_RBRACKET; } break;
        case ';': { token.tok_type = TK_SEMICOLON; } break;
        case '=': { 
            get_next_char(lex_state);
            if(peek_next_char(lex_state) == '=') {
                token.tok_type = TK_EQ_EQ;
            } else {
                token.tok_type = TK_EQ;
            }
        } break;
        case '~': { 
            get_next_char(lex_state);
            if(peek_next_char(lex_state) == '=') {
                token.tok_type = TK_NEQ;
            }
        } break;
        case '!': { token.tok_type = TK_BANG; } break;
        case '>': { 
            get_next_char(lex_state);
            if(peek_next_char(lex_state) == '=') {
                token.tok_type = TK_GT_EQ;
            } else {
                token.tok_type = TK_GT;
            }
        } break;
        case '<': { 
            get_next_char(lex_state);
            if(peek_next_char(lex_state) == '=') {
                token.tok_type = TK_LT_EQ;
            } else {
                token.tok_type = TK_LT;
            }
        } break;
        case '+': { token.tok_type = TK_ADD; } break;
        case '-': { 
            get_next_char(lex_state);
            if(peek_next_char(lex_state) == '-') {
                return make_comment_token(token, lex_state);
            } else {
                token.tok_type = TK_SUB;
            }
        } break;
        case '*': { token.tok_type = TK_MUL; } break;
        case '/': { token.tok_type = TK_DIV; } break;
        case '"': { 
            return make_string_token(token, lex_state);
        } break;
    };

    get_next_char(lex_state);
    return token;
}

void free_token(Token *token) {
    if(token->tok_type == TK_NAME || token->tok_type == TK_COMMENT || token->tok_type == TK_STRING) {
        if(token->info.str != NULL) {
            free((void*)token->info.str);
            token->info.str = NULL;
        }
    }
}

Token lpeek_next(LexerState *lex_state)
{
    if (lex_state->lookahead.tok_type != TK_EMPTY) {
        return lex_state->lookahead;
    }

    lex_state->lookahead = lnext_token(lex_state);
    return lex_state->lookahead;
}

Token lconsume(LexerState *lex_state)
{
    lex_state->lastline = lex_state->linenumber;
    if(lex_state->lookahead.tok_type != TK_EMPTY) {
        lex_state->tok = lex_state->lookahead;
        lex_state->lookahead.tok_type = TK_EMPTY;
        lex_state->lookahead.info.str = NULL;
    } else {
        lex_state->tok = lnext_token(lex_state);
    }
    return lex_state->tok;
}

LexerState *init_lexer(const char *filename)
{
    LexerState *lex_state = (LexerState *)malloc(sizeof(LexerState));
    FileBuffer *fbuffer = (FileBuffer *)malloc(sizeof(FileBuffer));
    FILE *f = fopen(filename, "rb");
    assert(f != NULL);

    char *buffer;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    assert(size >= 0);

    fbuffer->fsize = (size_t)size;
    fbuffer->location = 0;
    buffer = (char *)malloc(fbuffer->fsize + 1);
    if (buffer) {
        fread(buffer, 1, fbuffer->fsize, f);
    }
    fclose(f);

    buffer[size] = '\0'; // Null terminator
    fbuffer->buffer = buffer;

    // set everything to 0
    lex_state->linenumber = 0;
    lex_state->lastline = 0;
    lex_state->colnumber = 0;
    lex_state->tok = (Token){};
    lex_state->tok.tok_type = TK_EMPTY;
    lex_state->lookahead = (Token){};
    lex_state->lookahead.tok_type = TK_EMPTY;

    // set filename
    lex_state->fbuffer = fbuffer;
    lex_state->filename = filename;

    return lex_state;
}
