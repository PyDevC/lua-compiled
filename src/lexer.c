#include "lexer.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IsAlpha(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define IsDigit(c) (c >= '0' && c <= '9')
#define IsAlnum(c) (IsDigit(c) || IsAlpha(c))
#define IsBlank(c) (c == ' ' || c == '\t' || c == '\n' || c == '\r')
#define IsIdentifierStart(c) (IsAlpha(c) || c == '_')
#define IsIdentifier(c) (IsAlnum(c) || c == '_')

global const KeywordEntry keywords[] = {
    {TK_AND, "and"},       {TK_BREAK, "break"},   {TK_DO, "do"},
    {TK_ELSE, "else"},     {TK_ELSEIF, "elseif"}, {TK_END, "end"},
    {TK_FALSE, "false"},   {TK_FOR, "for"},       {TK_FUNCTION, "function"},
    {TK_IF, "if"},         {TK_IN, "in"},         {TK_LOCAL, "local"},
    {TK_NIL, "nil"},       {TK_NOT, "not"},       {TK_OR, "or"},
    {TK_REPEAT, "repeat"}, {TK_RETURN, "return"}, {TK_THEN, "then"},
    {TK_TRUE, "true"},     {TK_UNTIL, "until"},   {TK_WHILE, "while"}};

internal ReservedToken keywordType(const char *name)
{
    size_t num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < num_keywords; i++) {
        if (strcmp(keywords[i].name, name) == 0) {
            return keywords[i].type;
        }
    }
    return TK_EMPTY;
}

internal const char *keywordName(Token token)
{
    return keywords[token.tok_type].name;
}

const char *tokenToString(Token tok)
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
        return keywordName(tok);

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

internal char peekNextChar(LexerState *lex_state)
{
    size_t location = lex_state->fbuffer->location;
    if (location >= lex_state->fbuffer->fsize) {
        return EOF;
    }
    return lex_state->fbuffer->buffer[location];
}

internal char getNextChar(LexerState *lex_state)
{
    char c = peekNextChar(lex_state);
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

internal void skipWhiteSpace(LexerState *lex_state)
{
    while (IsBlank(peekNextChar(lex_state))) {
        getNextChar(lex_state);
    }
}

Token makeIdentifierToken(Token token, LexerState *lex_state)
{
    // TODO: set max identifier name limit
    // right now the hard limit is 63 chars
    char lname[64];
    int idx = 0;
    while (idx < 63 && IsIdentifier(peekNextChar(lex_state))) {
        lname[idx] = getNextChar(lex_state);
        idx++;
    }
    lname[idx] = '\0'; // mark end of string
    ReservedToken kw = keywordType(lname);
    if (kw != TK_EMPTY) {
        token.tok_type = kw;
        return token;
    }

    token.info.str = strdup(lname);
    token.tok_type = TK_NAME;
    return token;
}

Token makeNumberToken(Token token, LexerState *lex_state)
{
    char lname[64] = "";
    int idx = 0;
    while (IsDigit(peekNextChar(lex_state)) || peekNextChar(lex_state) == '.') {
        lname[idx] = getNextChar(lex_state);
        idx++;
    }
    lname[idx] = '\0'; // mark end of string
    token.info.number = atof(lname);
    token.tok_type = TK_NUMBER;
    return token;
}

Token makeStringToken(Token token, LexerState *lex_state)
{
    getNextChar(lex_state); // Consume opening quote

    char lname[200];
    int idx = 0;

    while (peekNextChar(lex_state) != '"' && peekNextChar(lex_state) != EOF) {
        if (idx < (int)sizeof(lname) - 2) {
            lname[idx++] = getNextChar(lex_state);
        } else {
            // String too long; consume rest or handle error
            getNextChar(lex_state);
        }
    }

    if (peekNextChar(lex_state) == '"') {
        getNextChar(lex_state); // Consume closing quote
    } else {
        token.tok_type = TK_ILLEGAL;
        return token;
    }

    lname[idx] = '\0';
    token.info.str = strdup(lname);
    token.tok_type = TK_STRING;
    return token;
}

Token makeCommentToken(Token token, LexerState *lex_state)
{
    char lname[200] = {'-', '-'};
    int idx = 0;
    getNextChar(lex_state);
    while (peekNextChar(lex_state) != '\n') {
        lname[idx] = getNextChar(lex_state);
        idx++;
    }
    lname[idx] = '\0';
    token.info.str = strdup(lname);
    token.tok_type = TK_COMMENT;
    return token;
}

// This will actually give you the token
Token lnextToken(LexerState *lex_state)
{
    Token token = {};
    token.tok_type = TK_ILLEGAL;
    token.info.str = NULL;

    skipWhiteSpace(lex_state);

    token.line = lex_state->linenumber;
    token.column = lex_state->colnumber;

    if (peekNextChar(lex_state) == EOF) {
        token.tok_type = TK_EOF;
        return token;
    }

    char c = peekNextChar(lex_state);
    if (IsIdentifier(c) || c == '"') {
        if (IsIdentifierStart(c)) {
            return makeIdentifierToken(token, lex_state);
        } else if (IsDigit(c)) {
            return makeNumberToken(token, lex_state);
        } else if (c == '"') {
            return makeStringToken(token, lex_state);
        }
    }

    // clang-format off
    int called_get_next = 0;
    switch (peekNextChar(lex_state)) {
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
            getNextChar(lex_state);
            called_get_next = 1;
            if(peekNextChar(lex_state) == '=') {
                getNextChar(lex_state);
                token.tok_type = TK_EQ_EQ;
            } else {
                token.tok_type = TK_EQ;
            }
        } break;
        case '~': { 
            getNextChar(lex_state);
            called_get_next = 1;
            if(peekNextChar(lex_state) == '=') {
                getNextChar(lex_state);
                token.tok_type = TK_NEQ;
            }
        } break;
        case '!': { token.tok_type = TK_BANG; } break;
        case '>': { 
            getNextChar(lex_state);
            called_get_next = 1;
            if(peekNextChar(lex_state) == '=') {
                getNextChar(lex_state);
                token.tok_type = TK_GT_EQ;
            } else {
                token.tok_type = TK_GT;
            }
        } break;
        case '<': { 
            getNextChar(lex_state);
            called_get_next = 1;
            if(peekNextChar(lex_state) == '=') {
                getNextChar(lex_state);
                token.tok_type = TK_LT_EQ;
            } else {
                token.tok_type = TK_LT;
            }
        } break;
        case '+': { token.tok_type = TK_ADD; } break;
        case '-': { 
            getNextChar(lex_state);
            called_get_next = 1;
            if(peekNextChar(lex_state) == '-') {
                getNextChar(lex_state);
                return makeCommentToken(token, lex_state);
            } else {
                token.tok_type = TK_SUB;
            }
        } break;
        case '*': { token.tok_type = TK_MUL; } break;
        case '/': { token.tok_type = TK_DIV; } break;
    };

    if(called_get_next == 0) {
        getNextChar(lex_state);
    }
    return token;
}

Token lpeekNext(LexerState *lex_state)
{
    if (lex_state->lookahead.tok_type != TK_EMPTY) {
        return lex_state->lookahead;
    }

    lex_state->lookahead = lnextToken(lex_state);
    return lex_state->lookahead;
}

Token lConsume(LexerState *lex_state)
{
    lex_state->lastline = lex_state->linenumber;
    if(lex_state->lookahead.tok_type != TK_EMPTY) {
        lex_state->tok = lex_state->lookahead;
        lex_state->lookahead.tok_type = TK_EMPTY;
        lex_state->lookahead.info.str = NULL;
    } else {
        lex_state->tok = lnextToken(lex_state);
    }
    return lex_state->tok;
}

LexerState *initLexer(const char *filename)
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

void destroyLexer(LexerState *lex_state)
{
    if (lex_state == NULL) {
        return;
    }
    if (lex_state->fbuffer != NULL) {
        free((void *)lex_state->fbuffer->buffer);
        free(lex_state->fbuffer);
    }
    free(lex_state);
}

#ifdef PARSER_DEBUG
void printToken(ReservedToken tok)
{
    switch (tok) {
    /* Keywords */
    case TK_AND: printf("and"); break;
    case TK_BREAK: printf("break"); break;
    case TK_DO: printf("do"); break;
    case TK_ELSE: printf("else"); break;
    case TK_ELSEIF: printf("elseif"); break;
    case TK_END: printf("end"); break;
    case TK_FALSE: printf("false"); break;
    case TK_FOR: printf("for"); break;
    case TK_FUNCTION: printf("function"); break;
    case TK_IF: printf("if"); break;
    case TK_IN: printf("in"); break;
    case TK_LOCAL: printf("local"); break;
    case TK_NIL: printf("nil"); break;
    case TK_NOT: printf("not"); break;
    case TK_OR: printf("or"); break;
    case TK_REPEAT: printf("repeat"); break;
    case TK_RETURN: printf("return"); break;
    case TK_THEN: printf("then"); break;
    case TK_TRUE: printf("true"); break;
    case TK_UNTIL: printf("until"); break;
    case TK_WHILE: printf("while"); break;
    /* Symbols */
    case TK_COMMA: printf(","); break;
    case TK_HASH: printf("#"); break;
    case TK_LPAREN: printf("("); break;
    case TK_RPAREN: printf(")"); break;
    case TK_LBRACE: printf("{"); break;
    case TK_RBRACE: printf("}"); break;
    case TK_LBRACKET: printf("["); break;
    case TK_RBRACKET: printf("]"); break;
    case TK_SEMICOLON: printf(";"); break;
    case TK_EQ: printf("="); break;
    case TK_EQ_EQ: printf("=="); break;
    case TK_NEQ: printf("~="); break;
    case TK_BANG: printf("!"); break;
    case TK_GT: printf(">"); break;
    case TK_GT_EQ: printf(">="); break;
    case TK_LT: printf("<"); break;
    case TK_LT_EQ: printf("<="); break;
    case TK_ADD: printf("+"); break;
    case TK_SUB: printf("-"); break;
    case TK_MUL: printf("*"); break;
    case TK_DIV: printf("/"); break;
    /* Types & Special */
    case TK_NUMBER: printf("<number>"); break;
    case TK_STRING: printf("<string>"); break;
    case TK_NAME: printf("<name>"); break;
    case TK_ILLEGAL: printf("<illegal>"); break;
    case TK_EOF: printf("<eof>"); break;
    case TK_EMPTY: printf("<empty>"); break;
    case TK_COMMENT: printf("<comment>"); break;
    default: printf("?"); break;
    }
}

#endif // PARSER_DEBUG
