/* ========================================================================

   $File: parser.c
   $Date: 2026/11/20
   $Revision: 1

   Pratt parser. Two dense arrays (ExprPool, StmtPool) are sized once from
   the source length, so parsing never reallocates and never frees. Nodes
   refer to each other by ASTIndex. Errors are recorded on the parser and
   printed with file/line/column; there is no error-code plumbing.

   ======================================================================== */

#include "parser.h"

#include <stdarg.h>

typedef ASTIndex (*PrefixFn)(Parser *, Token);
typedef ASTIndex (*InfixFn)(Parser *, ASTIndex, Token);

typedef struct ParserRule
{
    PrefixFn Prefix;
    InfixFn Infix;
    Precedence Prec;
} ParserRule;

internal ASTIndex ParseNilPrefix(Parser *P, Token Tok);
internal ASTIndex ParseBooleanPrefix(Parser *P, Token Tok);
internal ASTIndex ParseNumberPrefix(Parser *P, Token Tok);
internal ASTIndex ParseStringPrefix(Parser *P, Token Tok);
internal ASTIndex ParseNamePrefix(Parser *P, Token Tok);
internal ASTIndex ParseGroupPrefix(Parser *P, Token Tok);
internal ASTIndex ParseUnaryPrefix(Parser *P, Token Tok);
internal ASTIndex ParseBinaryInfix(Parser *P, ASTIndex Left, Token Op);
internal ASTIndex ParseCallInfix(Parser *P, ASTIndex Left, Token Op);

internal ASTIndex ParseExpression(Parser *P, Precedence MinPrec);
internal ASTIndex ParseStatement(Parser *P);
internal ASTIndex ParseBlock(Parser *P);

global_variable const ParserRule Rules[TK_COUNT] =
    {
        /* literals */
        [TK_NIL] = {ParseNilPrefix, 0, PREC_NONE},
        [TK_TRUE] = {ParseBooleanPrefix, 0, PREC_NONE},
        [TK_FALSE] = {ParseBooleanPrefix, 0, PREC_NONE},
        [TK_NUMBER] = {ParseNumberPrefix, 0, PREC_NONE},
        [TK_STRING] = {ParseStringPrefix, 0, PREC_NONE},
        [TK_NAME] = {ParseNamePrefix, 0, PREC_NONE},

        /* grouping and calls */
        [TK_LPAREN] = {ParseGroupPrefix, ParseCallInfix, PREC_CALL},

        /* unary */
        [TK_SUB] = {ParseUnaryPrefix, ParseBinaryInfix, PREC_TERM},
        [TK_NOT] = {ParseUnaryPrefix, 0, PREC_NONE},
        [TK_HASH] = {ParseUnaryPrefix, 0, PREC_NONE},

        /* arithmetic */
        [TK_ADD] = {0, ParseBinaryInfix, PREC_TERM},
        [TK_MUL] = {0, ParseBinaryInfix, PREC_FACTOR},
        [TK_DIV] = {0, ParseBinaryInfix, PREC_FACTOR},

        /* comparison */
        [TK_EQ_EQ] = {0, ParseBinaryInfix, PREC_COMPARE},
        [TK_NEQ] = {0, ParseBinaryInfix, PREC_COMPARE},
        [TK_LT] = {0, ParseBinaryInfix, PREC_COMPARE},
        [TK_LT_EQ] = {0, ParseBinaryInfix, PREC_COMPARE},
        [TK_GT] = {0, ParseBinaryInfix, PREC_COMPARE},
        [TK_GT_EQ] = {0, ParseBinaryInfix, PREC_COMPARE},

        /* logical */
        [TK_AND] = {0, ParseBinaryInfix, PREC_AND},
        [TK_OR] = {0, ParseBinaryInfix, PREC_OR},
    };

internal b32
IsBlockEnd(ReservedToken Type)
{
    b32 Result = ((Type == TK_END) || (Type == TK_ELSE) ||
        (Type == TK_ELSEIF) || (Type == TK_UNTIL) ||
        (Type == TK_EOF));
    return(Result);
}

internal void
ErrorAt(Parser *P, Token *Tok, char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    PushMessageV(P->Log, Msg_Error, P->Source, Tok->offset, "parse", Format,
                 Args);
    va_end(Args);
}

internal void
Advance(Parser *P)
{
    do
    {
        P->Curr = lconsume(&P->LexState);
    } while((P->Curr.tok_type == TK_COMMENT) ||
    (P->Curr.tok_type == TK_SEMICOLON));
}

internal b32
Match(Parser *P, ReservedToken Type)
{
    b32 Result = 0;
    if(P->Curr.tok_type == Type)
    {
        Advance(P);
        Result = 1;
    }
    return(Result);
}

internal b32
Expect(Parser *P, ReservedToken Type, char *What)
{
    b32 Result = 1;
    if(P->Curr.tok_type != Type)
    {
        ErrorAt(P, &P->Curr, "expected %s, got '%s'", What,
                token_to_string(P->Curr));
        Result = 0;
    }
    else
{
        Advance(P);
    }
    return(Result);
}

internal ASTIndex
AllocExpr(Parser *P, Expr Init, Token At)
{
    Assert(P->ExprLen < P->ExprCap);
    Init.next = ASTNone;
    Init.Offset = At.offset;
    Init.SymbolIndex = ASTNone;
    ASTIndex Result = P->ExprLen++;
    P->ExprPool[Result] = Init;
    return(Result);
}

internal ASTIndex
AllocStmt(Parser *P, Statement Init, Token At)
{
    Assert(P->StmtLen < P->StmtCap);
    Init.next = ASTNone;
    Init.Offset = At.offset;
    Init.SymbolIndex = ASTNone;
    ASTIndex Result = P->StmtLen++;
    P->StmtPool[Result] = Init;
    return(Result);
}

Expr *
GetExpr(Parser *P, ASTIndex Index)
{
    Assert(Index != ASTNone);
    Assert(Index < P->ExprLen);
    return(P->ExprPool + Index);
}

Statement *
GetStatement(Parser *P, ASTIndex Index)
{
    Assert(Index != ASTNone);
    Assert(Index < P->StmtLen);
    return(P->StmtPool + Index);
}

internal ASTIndex
ParseExpression(Parser *P, Precedence MinPrec)
{
    ASTIndex Result = ASTNone;
    PrefixFn Prefix = Rules[P->Curr.tok_type].Prefix;

    if(Prefix)
    {
        Token Tok = P->Curr;
        Advance(P);
        Result = Prefix(P, Tok);

        while((Result != ASTNone) &&
            (MinPrec <= Rules[P->Curr.tok_type].Prec))
        {
            InfixFn Infix = Rules[P->Curr.tok_type].Infix;
            if(!Infix)
            {
                break;
            }
            Token Op = P->Curr;
            Advance(P);
            Result = Infix(P, Result, Op);
        }
    }
    else
{
        ErrorAt(P, &P->Curr, "expected expression, got '%s'",
                token_to_string(P->Curr));
        Advance(P); /* recovery: always make progress */
    }

    return(Result);
}

internal ASTIndex
ParseNilPrefix(Parser *P, Token Tok)
{
    (void)Tok;
    Expr Init = {0};
    Init.type = ExprNil;
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseBooleanPrefix(Parser *P, Token Tok)
{
    Expr Init = {0};
    Init.type = ExprBoolean;
    Init.as.boolean = (Tok.tok_type == TK_TRUE);
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseNumberPrefix(Parser *P, Token Tok)
{
    Expr Init = {0};
    Init.type = ExprNumber;
    Init.as.num = Tok.info.num;
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseStringPrefix(Parser *P, Token Tok)
{
    Expr Init = {0};
    Init.type = ExprString;
    Init.as.string = Tok.info.str;
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseNamePrefix(Parser *P, Token Tok)
{
    Expr Init = {0};
    Init.type = ExprVariable;
    Init.as.variable = Tok.info.str;
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseGroupPrefix(Parser *P, Token Tok)
{
    (void)Tok;
    ASTIndex Inner = ParseExpression(P, PREC_ASSIGN);
    if(!Expect(P, TK_RPAREN, "')'"))
    {
        return(ASTNone);
    }

    Expr Init = {0};
    Init.type = ExprGroup;
    Init.as.group = Inner;
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseUnaryPrefix(Parser *P, Token Tok)
{
    ASTIndex Operand = ParseExpression(P, PREC_UNARY);
    Expr Init = {0};
    Init.type = ExprUnary;
    Init.as.unary.op = Tok.tok_type;
    Init.as.unary.l_expr = Operand;
    return(AllocExpr(P, Init, Tok));
}

internal ASTIndex
ParseBinaryInfix(Parser *P, ASTIndex Left, Token Op)
{
    ASTIndex Right = ParseExpression(P, (Precedence)(Rules[Op.tok_type].Prec + 1));
    Expr Init = {0};
    Init.type = ExprBinary;
    Init.as.binary.op = Op.tok_type;
    Init.as.binary.l_expr = Left;
    Init.as.binary.r_expr = Right;
    return(AllocExpr(P, Init, Op));
}

internal ASTIndex
ParseCallInfix(Parser *P, ASTIndex Left, Token Op)
{
    (void)Op;
    ASTIndex First = ASTNone;
    ASTIndex Last = ASTNone;
    u32 Argc = 0;

    if(P->Curr.tok_type != TK_RPAREN)
    {
        for(;;)
        {
            ASTIndex Arg = ParseExpression(P, PREC_ASSIGN);
            if(Arg == ASTNone)
            {
                return(ASTNone);
            }
            if(First == ASTNone)
            {
                First = Arg;
            }
            else
        {
                GetExpr(P, Last)->next = Arg;
            }
            Last = Arg;
            Argc++;

            if(!Match(P, TK_COMMA))
            {
                break;
            }
        }
    }

    if(!Expect(P, TK_RPAREN, "')'"))
    {
        return(ASTNone);
    }

    Expr Init = {0};
    Init.type = ExprFuncCall;
    Init.as.call.callee = Left;
    Init.as.call.i_arg = First;
    Init.as.call.argc = Argc;
    return(AllocExpr(P, Init, Op));
}

//
// Statements
//

internal ASTIndex
ParseNameStatement(Parser *P)
{
    Token Start = P->Curr;
    ASTIndex ExprIndex = ParseExpression(P, PREC_ASSIGN);
    if(ExprIndex == ASTNone)
    {
        return(ASTNone);
    }

    Expr *E = GetExpr(P, ExprIndex);
    if(Match(P, TK_EQ))
    {
        if(E->type != ExprVariable)
        {
            ErrorAt(P, &P->Curr, "left side of '=' must be a name");
            return(ASTNone);
        }
        const char *Name = E->as.variable;
        ASTIndex Value = ParseExpression(P, PREC_ASSIGN);
        Statement Init = {0};
        Init.type = StmtAssignment;
        Init.as.assign.name = Name;
        Init.as.assign.value = Value;
        return(AllocStmt(P, Init, Start));
    }

    if(E->type != ExprFuncCall)
    {
        ErrorAt(P, &P->Curr, "expected '=' or a function call after expression");
        return(ASTNone);
    }

    Statement Init = {0};
    Init.type = StmtFuncCall;
    Init.as.funccall = ExprIndex;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseLocalStatement(Parser *P)
{
    Token Start = P->Curr;
    Advance(P); /* local */
    if(P->Curr.tok_type != TK_NAME)
    {
        ErrorAt(P, &P->Curr, "expected variable name after 'local'");
        Advance(P);
        return(ASTNone);
    }

    const char *Name = P->Curr.info.str;
    Advance(P);

    ASTIndex Value = ASTNone;
    if(Match(P, TK_EQ))
    {
        Value = ParseExpression(P, PREC_ASSIGN);
    }

    Statement Init = {0};
    Init.type = StmtLocalVar;
    Init.as.local_var.name = Name;
    Init.as.local_var.value = Value;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseIfThen(Parser *P)
{
    Token Start = P->Curr;
    ASTIndex Cond = ParseExpression(P, PREC_ASSIGN);
    if(!Expect(P, TK_THEN, "'then'"))
    {
        return(ASTNone);
    }
    ASTIndex Body = ParseBlock(P);

    Statement Init = {0};
    Init.type = StmtIfElse;
    Init.as.if_stmt.cond = Cond;
    Init.as.if_stmt.block_idx = Body;
    Init.as.if_stmt.next_el = ASTNone;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseIfStatement(Parser *P)
{
    Advance(P); /* if */
    ASTIndex First = ParseIfThen(P);
    if(First == ASTNone)
    {
        return(ASTNone);
    }

    ASTIndex LastIf = First;
    while(Match(P, TK_ELSEIF))
    {
        ASTIndex ElseIfIndex = ParseIfThen(P);
        if(ElseIfIndex == ASTNone)
        {
            break;
        }
        GetStatement(P, LastIf)->as.if_stmt.next_el = ElseIfIndex;
        LastIf = ElseIfIndex;
    }

    Token ElseTok = P->Curr;
    if(Match(P, TK_ELSE))
    {
        ASTIndex ElseBody = ParseBlock(P);
        Statement Init = {0};
        Init.type = StmtIfElse;
        Init.as.if_stmt.cond = ASTNone; /* ASTNone marks the else arm */
        Init.as.if_stmt.block_idx = ElseBody;
        Init.as.if_stmt.next_el = ASTNone;
        ASTIndex ElseIndex = AllocStmt(P, Init, ElseTok);
        GetStatement(P, LastIf)->as.if_stmt.next_el = ElseIndex;
    }

    Expect(P, TK_END, "'end'");
    return(First);
}

internal ASTIndex
ParseWhileStatement(Parser *P)
{
    Token Start = P->Curr;
    Advance(P); /* while */
    ASTIndex Cond = ParseExpression(P, PREC_ASSIGN);
    if(!Expect(P, TK_DO, "'do'"))
    {
        return(ASTNone);
    }
    ASTIndex Body = ParseBlock(P);
    Expect(P, TK_END, "'end'");

    Statement Init = {0};
    Init.type = StmtWhile;
    Init.as.while_or_repeat_stmt.cond = Cond;
    Init.as.while_or_repeat_stmt.block_idx = Body;
    Init.as.while_or_repeat_stmt.is_repeat_until = 0;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseRepeatStatement(Parser *P)
{
    Token Start = P->Curr;
    Advance(P); /* repeat */
    ASTIndex Body = ParseBlock(P);
    if(!Expect(P, TK_UNTIL, "'until'"))
    {
        return(ASTNone);
    }
    ASTIndex Cond = ParseExpression(P, PREC_ASSIGN);

    Statement Init = {0};
    Init.type = StmtRepeatUntil;
    Init.as.while_or_repeat_stmt.cond = Cond;
    Init.as.while_or_repeat_stmt.block_idx = Body;
    Init.as.while_or_repeat_stmt.is_repeat_until = 1;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseForStatement(Parser *P)
{
    Token Start = P->Curr;
    Advance(P); /* for */
    if(P->Curr.tok_type != TK_NAME)
    {
        ErrorAt(P, &P->Curr, "expected loop variable after 'for'");
        return(ASTNone);
    }
    const char *Var = P->Curr.info.str;
    Advance(P);

    if(!Expect(P, TK_EQ, "'='"))
    {
        return(ASTNone);
    }
    ASTIndex StartValue = ParseExpression(P, PREC_ASSIGN);
    if(!Expect(P, TK_COMMA, "','"))
    {
        return(ASTNone);
    }
    ASTIndex End = ParseExpression(P, PREC_ASSIGN);

    ASTIndex Step = ASTNone;
    if(Match(P, TK_COMMA))
    {
        Step = ParseExpression(P, PREC_ASSIGN);
    }

    if(!Expect(P, TK_DO, "'do'"))
    {
        return(ASTNone);
    }
    ASTIndex Body = ParseBlock(P);
    Expect(P, TK_END, "'end'");

    Statement Init = {0};
    Init.type = StmtForLoop;
    Init.as.for_stmt.var = Var;
    Init.as.for_stmt.start = StartValue;
    Init.as.for_stmt.end_ = End;
    Init.as.for_stmt.step = Step;
    Init.as.for_stmt.block_idx = Body;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseFunctionStatement(Parser *P)
{
    Token Start = P->Curr;
    Advance(P); /* function */
    if(P->Curr.tok_type != TK_NAME)
    {
        ErrorAt(P, &P->Curr, "expected function name");
        return(ASTNone);
    }
    const char *Name = P->Curr.info.str;
    Advance(P);

    if(!Expect(P, TK_LPAREN, "'('"))
    {
        return(ASTNone);
    }

    ASTIndex FirstParam = ASTNone;
    ASTIndex LastParam = ASTNone;
    u32 ParamCount = 0;
    if(P->Curr.tok_type != TK_RPAREN)
    {
        for(;;)
        {
            if(P->Curr.tok_type != TK_NAME)
            {
                ErrorAt(P, &P->Curr, "expected parameter name");
                return(ASTNone);
            }
            Token ParamTok = P->Curr;
            Expr Init = {0};
            Init.type = ExprVariable;
            Init.as.variable = ParamTok.info.str;
            ASTIndex Param = AllocExpr(P, Init, ParamTok);
            if(FirstParam == ASTNone)
            {
                FirstParam = Param;
            }
            else
        {
                GetExpr(P, LastParam)->next = Param;
            }
            LastParam = Param;
            ParamCount++;
            Advance(P);

            if(!Match(P, TK_COMMA))
            {
                break;
            }
        }
    }
    if(!Expect(P, TK_RPAREN, "')'"))
    {
        return(ASTNone);
    }

    ASTIndex Body = ParseBlock(P);
    Expect(P, TK_END, "'end'");

    Statement Init = {0};
    Init.type = StmtFunction;
    Init.as.func_stmt.name = Name;
    Init.as.func_stmt.first_arg = FirstParam;
    Init.as.func_stmt.argc = ParamCount;
    Init.as.func_stmt.first_stmt = Body;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseReturnStatement(Parser *P)
{
    Token Start = P->Curr;
    Advance(P); /* return */
    ASTIndex Value = ASTNone;
    if(!IsBlockEnd(P->Curr.tok_type))
    {
        Value = ParseExpression(P, PREC_ASSIGN);
    }

    Statement Init = {0};
    Init.type = StmtReturn;
    Init.as.return_value = Value;
    return(AllocStmt(P, Init, Start));
}

internal ASTIndex
ParseStatement(Parser *P)
{
    Token Start = P->Curr;
    ASTIndex Result = ASTNone;
    switch(P->Curr.tok_type)
    {
        case TK_NAME:     { Result = ParseNameStatement(P); } break;
        case TK_LOCAL:    { Result = ParseLocalStatement(P); } break;
        case TK_IF:       { Result = ParseIfStatement(P); } break;
        case TK_WHILE:    { Result = ParseWhileStatement(P); } break;
        case TK_REPEAT:   { Result = ParseRepeatStatement(P); } break;
        case TK_FOR:      { Result = ParseForStatement(P); } break;
        case TK_FUNCTION: { Result = ParseFunctionStatement(P); } break;
        case TK_RETURN:   { Result = ParseReturnStatement(P); } break;
        case TK_BREAK:
            {
                Advance(P);
                Statement Init = {0};
                Init.type = StmtBreak;
                Result = AllocStmt(P, Init, Start);
            } break;
        case TK_DO:
            {
                Advance(P);
                ASTIndex Body = ParseBlock(P);
                Expect(P, TK_END, "'end'");
                Statement Init = {0};
                Init.type = StmtBlock;
                Init.as.block.block_idx = Body;
                Result = AllocStmt(P, Init, Start);
            } break;

        default:
            {
                ErrorAt(P, &P->Curr, "unexpected token '%s'",
                        token_to_string(P->Curr));
                Advance(P);
            } break;
    }
    return(Result);
}

internal ASTIndex
ParseBlock(Parser *P)
{
    ASTIndex First = ASTNone;
    ASTIndex Last = ASTNone;
    while(!IsBlockEnd(P->Curr.tok_type))
    {
        ASTIndex Stmt = ParseStatement(P);
        if(Stmt == ASTNone)
        {
            continue;
        }
        if(First == ASTNone)
        {
            First = Stmt;
        }
        else
    {
            GetStatement(P, Last)->next = Stmt;
        }
        Last = Stmt;
    }
    return(First);
}

void
InitParser(Parser *P, memory_arena *Arena, source_file *Source,
           message_log *Log)
{
    ZeroStruct(*P);
    P->Arena = Arena;
    P->Source = Source;
    P->Log = Log;
    P->StartErrorCount = Log ? Log->List.ErrorCount : 0;
    InitLexer(&P->LexState, Arena, Source);

    /* Worst case: one AST node per source byte. No realloc, ever. */
    u32 MaxNodes = Source->Size + 16;
    P->ExprCap = MaxNodes;
    P->StmtCap = MaxNodes;
    P->ExprPool = PushArray(Arena, MaxNodes, Expr);
    P->StmtPool = PushArray(Arena, MaxNodes, Statement);

    Advance(P);
}

ASTIndex
ParseChunk(Parser *P)
{
    ASTIndex First = ASTNone;
    ASTIndex Last = ASTNone;
    while(P->Curr.tok_type != TK_EOF)
    {
        ASTIndex Stmt = ParseStatement(P);
        if(Stmt == ASTNone)
        {
            continue;
        }
        if(First == ASTNone)
        {
            First = Stmt;
        }
        else
    {
            GetStatement(P, Last)->next = Stmt;
        }
        Last = Stmt;
    }
    return(First);
}

b32
ParserOK(Parser *P)
{
    b32 Result = P->Log ? (P->Log->List.ErrorCount == P->StartErrorCount) : 1;
    return(Result);
}

//
// AST dump (-d). Kept in the same file so the debug path has no separate
// dependency graph; it is just another walker over the flat arrays.
//

void
DumpExpr(Parser *P, ASTIndex Index)
{
    if(Index == ASTNone)
    {
        printf("<none>");
        return;
    }

    Expr *E = GetExpr(P, Index);
    switch(E->type)
    {
        case ExprNil:      { printf("nil"); } break;
        case ExprBoolean:  { printf("%s", E->as.boolean ? "true" : "false"); } break;
        case ExprNumber:   { printf("%g", E->as.num); } break;
        case ExprString:   { printf("\"%s\"", E->as.string); } break;
        case ExprVariable: { printf("%s", E->as.variable); } break;
        case ExprGroup:
            {
                printf("(");
                DumpExpr(P, E->as.group);
                printf(")");
            } break;
        case ExprUnary:
            {
                Token Op = {0};
                Op.tok_type = E->as.unary.op;
                printf("%s", token_to_string(Op));
                DumpExpr(P, E->as.unary.l_expr);
            } break;
        case ExprBinary:
            {
                DumpExpr(P, E->as.binary.l_expr);
                Token Op = {0};
                Op.tok_type = E->as.binary.op;
                printf(" %s ", token_to_string(Op));
                DumpExpr(P, E->as.binary.r_expr);
            } break;
        case ExprFuncCall:
            {
                DumpExpr(P, E->as.call.callee);
                printf("(");
                ASTIndex Arg = E->as.call.i_arg;
                for(u32 ArgIndex = 0; ArgIndex < E->as.call.argc; ++ArgIndex)
                {
                    if(ArgIndex)
                    {
                        printf(", ");
                    }
                    DumpExpr(P, Arg);
                    Arg = GetExpr(P, Arg)->next;
                }
                printf(")");
            } break;

            InvalidDefaultCase;
    }
}

void
DumpStatement(Parser *P, ASTIndex Index)
{
    if(Index == ASTNone)
    {
        return;
    }

    Statement *S = GetStatement(P, Index);
    switch(S->type)
    {
        case StmtBlock:
            {
                printf("do\n");
                DumpStatement(P, S->as.block.block_idx);
                printf("end\n");
            } break;
        case StmtLocalVar:
            {
                printf("local %s = ", S->as.local_var.name);
                DumpExpr(P, S->as.local_var.value);
                printf("\n");
            } break;
        case StmtAssignment:
            {
                printf("%s = ", S->as.assign.name);
                DumpExpr(P, S->as.assign.value);
                printf("\n");
            } break;
        case StmtExpr:
            {
                DumpExpr(P, S->as.expr_stmt);
                printf("\n");
            } break;
        case StmtIfElse:
            {
                if(S->as.if_stmt.cond != ASTNone)
                {
                    printf("if ");
                    DumpExpr(P, S->as.if_stmt.cond);
                    printf(" then\n");
                }
                else
            {
                    printf("else\n");
                }
                DumpStatement(P, S->as.if_stmt.block_idx);
                DumpStatement(P, S->as.if_stmt.next_el);
            } break;
        case StmtWhile:
            {
                Assert(!S->as.while_or_repeat_stmt.is_repeat_until);
                printf("while ");
                DumpExpr(P, S->as.while_or_repeat_stmt.cond);
                printf(" do\n");
                DumpStatement(P, S->as.while_or_repeat_stmt.block_idx);
                printf("end\n");
            } break;
        case StmtRepeatUntil:
            {
                Assert(S->as.while_or_repeat_stmt.is_repeat_until);
                printf("repeat\n");
                DumpStatement(P, S->as.while_or_repeat_stmt.block_idx);
                printf("until ");
                DumpExpr(P, S->as.while_or_repeat_stmt.cond);
                printf("\n");
            } break;
        case StmtForLoop:
            {
                printf("for %s = ", S->as.for_stmt.var);
                DumpExpr(P, S->as.for_stmt.start);
                printf(", ");
                DumpExpr(P, S->as.for_stmt.end_);
                if(S->as.for_stmt.step != ASTNone)
                {
                    printf(", ");
                    DumpExpr(P, S->as.for_stmt.step);
                }
                printf(" do\n");
                DumpStatement(P, S->as.for_stmt.block_idx);
                printf("end\n");
            } break;
        case StmtFuncCall:
            {
                DumpExpr(P, S->as.funccall);
                printf("\n");
            } break;
        case StmtFunction:
            {
                printf("function %s(", S->as.func_stmt.name);
                ASTIndex Arg = S->as.func_stmt.first_arg;
                for(u32 ArgIndex = 0; ArgIndex < S->as.func_stmt.argc; ++ArgIndex)
                {
                    if(ArgIndex)
                    {
                        printf(", ");
                    }
                    DumpExpr(P, Arg);
                    Arg = GetExpr(P, Arg)->next;
                }
                printf(")\n");
                DumpStatement(P, S->as.func_stmt.first_stmt);
                printf("end\n");
            } break;
        case StmtReturn:
            {
                printf("return ");
                DumpExpr(P, S->as.return_value);
                printf("\n");
            } break;
        case StmtBreak:
            {
                printf("break\n");
            } break;

            InvalidDefaultCase;
    }

    DumpStatement(P, S->next);
}
