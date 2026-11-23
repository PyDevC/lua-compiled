/* ========================================================================

   $File: lexer.c
   $Date: 2026/11/20
   $Revision: 1

   This is the Casey-style lexer we started from. The only structural change
   is that CopySliceToCString pushes into the compile arena instead of
   malloc'ing: a token's text lives exactly as long as the compiler does.

   ======================================================================== */

#include "lexer.h"

/* SWAR helpers. x86-64 is little-endian, so byte i of Word is byte i of the
 * source. A 64-bit load scans eight identifier characters or whitespace
 * characters per iteration without touching the arena or the token pools. */
#define SWAR_ONES 0x0101010101010101ull
#define SWAR_HIGH 0x8080808080808080ull
#define SWAR_HASZERO(X) (((X) - SWAR_ONES) & ~(X) & SWAR_HIGH)
#define SWAR_HASLESS(X, N)                                                     \
    (((X) - (SWAR_ONES * (u64)(N))) & ~(X) & SWAR_HIGH)
#define SWAR_GE(X, N) (~SWAR_HASLESS(X, N) & SWAR_HIGH)
#define SWAR_LE(X, N) SWAR_HASLESS(X, (N) + 1)

internal b32 IsIdentChar(char C);
internal void AdvanceCursor(lexer_cursor *Cursor, u32 Count);

internal u64
LoadU64(const char *At)
{
    u64 Result = 0;
    memcpy(&Result, At, sizeof(Result));
    return(Result);
}

internal u64
LoadPaddedU64(const char *At, u32 Length)
{
    u64 Result = 0;
    u32 Count = Minimum(Length, 8);
    for(u32 Index = 0; Index < Count; ++Index)
    {
        Result |= ((u64)(u8)At[Index] << (8 * Index));
    }
    return(Result);
}

internal u64
IdentifierCharMask(u64 Word)
{
    u64 Digit = SWAR_GE(Word, '0') & SWAR_LE(Word, '9');
    u64 Folded = Word | (SWAR_ONES * 0x20);
    u64 Letter = SWAR_GE(Folded, 'a') & SWAR_LE(Folded, 'z');
    u64 Underscore = SWAR_HASZERO(Word ^ (SWAR_ONES * '_'));
    return((Digit | Letter | Underscore) & SWAR_HIGH);
}

internal u64
WhitespaceMask(u64 Word)
{
    u64 Space = SWAR_HASZERO(Word ^ (SWAR_ONES * ' '));
    u64 Tab = SWAR_HASZERO(Word ^ (SWAR_ONES * '\t'));
    u64 LineFeed = SWAR_HASZERO(Word ^ (SWAR_ONES * '\n'));
    u64 Carriage = SWAR_HASZERO(Word ^ (SWAR_ONES * '\r'));
    return((Space | Tab | LineFeed | Carriage) & SWAR_HIGH);
}

internal u32
FirstNonIdentifierByte(u64 Mask)
{
    u32 Result = 8;
    for(u32 Index = 0; Index < 8; ++Index)
    {
        if((~Mask & SWAR_HIGH) & ((u64)0x80 << (8 * Index)))
        {
            Result = Index;
            break;
        }
    }
    return(Result);
}

internal void
AdvanceWhitespaceWord(lexer_cursor *Cursor)
{
    Cursor->At += 8;
}

internal void
SkipIdentifierRun(lexer_cursor *Cursor)
{
    while((u64)(Cursor->End - Cursor->At) >= 8)
    {
        u64 Word = LoadU64(Cursor->At);
        u64 Mask = IdentifierCharMask(Word);
        if(Mask != SWAR_HIGH)
        {
            u32 Count = FirstNonIdentifierByte(Mask);
            Cursor->At += Count;
            return;
        }
        Cursor->At += 8;
    }

    while((Cursor->At < Cursor->End) && IsIdentChar(*Cursor->At))
    {
        AdvanceCursor(Cursor, 1);
    }
}

internal b32
IsAlpha(char C)
{
    b32 Result = ((C >= 'a') && (C <= 'z')) || ((C >= 'A') && (C <= 'Z'));
    return(Result);
}

internal b32
IsDigit(char C)
{
    b32 Result = ((C >= '0') && (C <= '9'));
    return(Result);
}

internal b32
IsWhitespace(char C)
{
    b32 Result = ((C == ' ') || (C == '\t') || (C == '\n') || (C == '\r'));
    return(Result);
}

internal b32
IsIdentStart(char C)
{
    b32 Result = (IsAlpha(C) || (C == '_'));
    return(Result);
}

internal b32
IsIdentChar(char C)
{
    b32 Result = (IsAlpha(C) || IsDigit(C) || (C == '_'));
    return(Result);
}

global_variable const KeywordEntry GlobalKeywords[] =
    {
        {TK_AND, "and"},           {TK_BREAK, "break"},   {TK_DO, "do"},
        {TK_ELSE, "else"},         {TK_ELSEIF, "elseif"}, {TK_END, "end"},
        {TK_FALSE, "false"},       {TK_FOR, "for"},       {TK_FUNCTION, "function"},
        {TK_IF, "if"},             {TK_IN, "in"},         {TK_LOCAL, "local"},
        {TK_NIL, "nil"},           {TK_NOT, "not"},       {TK_OR, "or"},
        {TK_REPEAT, "repeat"},     {TK_RETURN, "return"}, {TK_THEN, "then"},
        {TK_TRUE, "true"},         {TK_UNTIL, "until"},   {TK_WHILE, "while"},
    };

internal ReservedToken
LookupKeyword(const char *Start, u32 Length)
{
    ReservedToken Result = TK_EMPTY;
    u64 Needle = LoadPaddedU64(Start, Length);
    for(u32 Index = 0; Index < ArrayCount(GlobalKeywords); ++Index)
    {
        const KeywordEntry *Entry = GlobalKeywords + Index;
        if(((u32)strlen(Entry->name) == Length) &&
            (LoadPaddedU64(Entry->name, Length) == Needle))
        {
            Result = Entry->type;
            break;
        }
    }
    return(Result);
}

internal const char *
KeywordString(ReservedToken Type)
{
    const char *Result = "";
    for(u32 Index = 0; Index < ArrayCount(GlobalKeywords); ++Index)
    {
        if(GlobalKeywords[Index].type == Type)
        {
            Result = GlobalKeywords[Index].name;
            break;
        }
    }
    return(Result);
}

internal void
AdvanceCursor(lexer_cursor *Cursor, u32 Count)
{
    Cursor->At += Minimum(Count, (u32)(Cursor->End - Cursor->At));
}

internal void
SkipWhitespace(lexer_cursor *Cursor)
{
    while((u64)(Cursor->End - Cursor->At) >= 8)
    {
        u64 Word = LoadU64(Cursor->At);
        if(WhitespaceMask(Word) != SWAR_HIGH)
        {
            break;
        }
        AdvanceWhitespaceWord(Cursor);
    }

    while((Cursor->At < Cursor->End) && IsWhitespace(*Cursor->At))
    {
        AdvanceCursor(Cursor, 1);
    }
}

internal char *
CopySliceToCString(lexer_cursor *Cursor, const char *Start, u32 Length)
{
    Assert(Cursor->Arena);
    char *Result = PushArray(Cursor->Arena, Length + 1, char);
    memcpy(Result, Start, Length);
    Result[Length] = 0;
    return(Result);
}

Token
ScanNextToken(lexer_cursor *Cursor)
{
    SkipWhitespace(Cursor);

    Token Result = {0};
    Result.tok_type = TK_ILLEGAL;
    Result.offset = (u32)(Cursor->At - Cursor->Base);

    if(Cursor->At >= Cursor->End)
    {
        Result.tok_type = TK_EOF;
        return(Result);
    }

    char C = Cursor->At[0];

    if(IsIdentStart(C))
    {
        const char *Start = Cursor->At;
        SkipIdentifierRun(Cursor);
        u32 Length = (u32)(Cursor->At - Start);
        ReservedToken Keyword = LookupKeyword(Start, Length);
        if(Keyword != TK_EMPTY)
        {
            Result.tok_type = Keyword;
        }
        else
    {
            Result.tok_type = TK_NAME;
            Result.info.str = CopySliceToCString(Cursor, Start, Length);
        }
        return(Result);
    }

    if(IsDigit(C))
    {
        /* Accumulate directly. No fixed-size buffer and no atof on a slice. */
        f64 Number = 0;
        while((Cursor->At < Cursor->End) && IsDigit(*Cursor->At))
        {
            Number = Number * 10 + (f64)(*Cursor->At - '0');
            AdvanceCursor(Cursor, 1);
        }

        if((Cursor->At < Cursor->End) && (*Cursor->At == '.'))
        {
            AdvanceCursor(Cursor, 1);
            f64 Scale = 10;
            while((Cursor->At < Cursor->End) && IsDigit(*Cursor->At))
            {
                Number += (f64)(*Cursor->At - '0') / Scale;
                Scale *= 10;
                AdvanceCursor(Cursor, 1);
            }
        }

        Result.tok_type = TK_NUMBER;
        Result.info.num = Number;
        return(Result);
    }

    if(C == '"')
    {
        AdvanceCursor(Cursor, 1); // Opening quote
        const char *Start = Cursor->At;
        while((Cursor->At < Cursor->End) && (*Cursor->At != '"'))
        {
            AdvanceCursor(Cursor, 1);
        }

        if((Cursor->At < Cursor->End) && (*Cursor->At == '"'))
        {
            u32 Length = (u32)(Cursor->At - Start);
            Result.tok_type = TK_STRING;
            Result.info.str = CopySliceToCString(Cursor, Start, Length);
            AdvanceCursor(Cursor, 1); // Closing quote
        }
        else
    {
            Result.tok_type = TK_ILLEGAL;
        }
        return(Result);
    }

    switch(C)
    {
        case ',': { Result.tok_type = TK_COMMA;     AdvanceCursor(Cursor, 1); } break;
        case '#': { Result.tok_type = TK_HASH;      AdvanceCursor(Cursor, 1); } break;
        case '(': { Result.tok_type = TK_LPAREN;    AdvanceCursor(Cursor, 1); } break;
        case ')': { Result.tok_type = TK_RPAREN;    AdvanceCursor(Cursor, 1); } break;
        case '{': { Result.tok_type = TK_LBRACE;    AdvanceCursor(Cursor, 1); } break;
        case '}': { Result.tok_type = TK_RBRACE;    AdvanceCursor(Cursor, 1); } break;
        case '[': { Result.tok_type = TK_LBRACKET;  AdvanceCursor(Cursor, 1); } break;
        case ']': { Result.tok_type = TK_RBRACKET;  AdvanceCursor(Cursor, 1); } break;
        case ';': { Result.tok_type = TK_SEMICOLON; AdvanceCursor(Cursor, 1); } break;
        case '!': { Result.tok_type = TK_BANG;      AdvanceCursor(Cursor, 1); } break;
        case '+': { Result.tok_type = TK_ADD;       AdvanceCursor(Cursor, 1); } break;
        case '*': { Result.tok_type = TK_MUL;       AdvanceCursor(Cursor, 1); } break;
        case '/': { Result.tok_type = TK_DIV;       AdvanceCursor(Cursor, 1); } break;

        case '=':
            {
                if(((Cursor->At + 1) < Cursor->End) && (Cursor->At[1] == '='))
                {
                    Result.tok_type = TK_EQ_EQ;
                    AdvanceCursor(Cursor, 2);
                }
                else
            {
                    Result.tok_type = TK_EQ;
                    AdvanceCursor(Cursor, 1);
                }
            } break;

        case '~':
            {
                if(((Cursor->At + 1) < Cursor->End) && (Cursor->At[1] == '='))
                {
                    Result.tok_type = TK_NEQ;
                    AdvanceCursor(Cursor, 2);
                }
                else
            {
                    AdvanceCursor(Cursor, 1);
                }
            } break;

        case '>':
            {
                if(((Cursor->At + 1) < Cursor->End) && (Cursor->At[1] == '='))
                {
                    Result.tok_type = TK_GT_EQ;
                    AdvanceCursor(Cursor, 2);
                }
                else
            {
                    Result.tok_type = TK_GT;
                    AdvanceCursor(Cursor, 1);
                }
            } break;

        case '<':
            {
                if(((Cursor->At + 1) < Cursor->End) && (Cursor->At[1] == '='))
                {
                    Result.tok_type = TK_LT_EQ;
                    AdvanceCursor(Cursor, 2);
                }
                else
            {
                    Result.tok_type = TK_LT;
                    AdvanceCursor(Cursor, 1);
                }
            } break;

        case '-':
            {
                if(((Cursor->At + 1) < Cursor->End) && (Cursor->At[1] == '-'))
                {
                    AdvanceCursor(Cursor, 2);
                    const char *Start = Cursor->At;
                    while((Cursor->At < Cursor->End) && (*Cursor->At != '\n'))
                    {
                        AdvanceCursor(Cursor, 1);
                    }
                    u32 Length = (u32)(Cursor->At - Start);
                    Result.tok_type = TK_COMMENT;
                    Result.info.str = CopySliceToCString(Cursor, Start, Length);
                }
                else
            {
                    Result.tok_type = TK_SUB;
                    AdvanceCursor(Cursor, 1);
                }
            } break;

        default:
            {
                AdvanceCursor(Cursor, 1);
            } break;
    }

    return(Result);
}

const char *
token_to_string(Token Tok)
{
    switch(Tok.tok_type)
    {
        case TK_EOF:
        case TK_EMPTY:   return("");

        case TK_ILLEGAL:
        case TK_COMMENT:
        case TK_STRING:
        case TK_NAME:    return(Tok.info.str ? Tok.info.str : "");

        case TK_NUMBER:  return("[NUMBER]");

        case TK_AND: case TK_BREAK: case TK_DO: case TK_ELSE: case TK_ELSEIF:
        case TK_END: case TK_FALSE: case TK_FOR: case TK_FUNCTION: case TK_IF:
        case TK_IN: case TK_LOCAL: case TK_NIL: case TK_NOT: case TK_OR:
        case TK_REPEAT: case TK_RETURN: case TK_THEN: case TK_TRUE:
        case TK_UNTIL: case TK_WHILE: return(KeywordString(Tok.tok_type));

        case TK_COMMA:     return(",");
        case TK_HASH:      return("#");
        case TK_LPAREN:    return("(");
        case TK_RPAREN:    return(")");
        case TK_LBRACE:    return("{");
        case TK_RBRACE:    return("}");
        case TK_LBRACKET:  return("[");
        case TK_RBRACKET:  return("]");
        case TK_SEMICOLON: return(";");
        case TK_EQ:        return("=");
        case TK_EQ_EQ:     return("==");
        case TK_NEQ:       return("~=");
        case TK_BANG:      return("!");
        case TK_GT:        return(">");
        case TK_GT_EQ:     return(">=");
        case TK_LT:        return("<");
        case TK_LT_EQ:     return("<=");
        case TK_ADD:       return("+");
        case TK_SUB:       return("-");
        case TK_MUL:       return("*");
        case TK_DIV:       return("/");

            InvalidDefaultCase;
    }

    return("");
}

void
InitLexer(LexState *LexState, memory_arena *Arena, source_file *Source)
{
    ZeroStruct(*LexState);
    LexState->Cursor.Base = Source->Data;
    LexState->Cursor.At = Source->Data;
    LexState->Cursor.End = Source->Data + Source->Size;
    LexState->Cursor.Arena = Arena;
    LexState->Lookahead.tok_type = TK_EMPTY;
}

Token
lconsume(LexState *LexState)
{
    Token Result = {};
    if(LexState->HasLookahead)
    {
        Result = LexState->Lookahead;
        LexState->Lookahead = (Token){0};
        LexState->HasLookahead = 0;
    }
    else
{
        Result = ScanNextToken(&LexState->Cursor);
    }
    LexState->Curr = Result;
    return(Result);
}

Token
lpeek_next(LexState *LexState)
{
    if(!LexState->HasLookahead)
    {
        LexState->Lookahead = ScanNextToken(&LexState->Cursor);
        LexState->HasLookahead = 1;
    }
    return(LexState->Lookahead);
}
