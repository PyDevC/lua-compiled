/* ========================================================================

   $File: lua.c
   $Date: 2026/11/20
   $Revision: 1

   Platform layer. Like every Handmade Hero platform file, this is the ONLY
   translation unit: it includes the game-side .c files below, so internal
   helpers can stay internal and there is one compiler invocation.

   The pipeline is now parse -> semantic/type check -> typed IR -> AMD64
   assembly. There is no interpreter in the executable path.

   Build: gcc -std=c11 -O3 -Wall -Wextra -Werror src/lua.c -o build/lua
   Usage: lua [-d] [-i] [-p] [-v] [-s file.s] file.lua

   ======================================================================== */

#include <time.h>

#include "defs.h"
#include "lexer.h"
#include "last.h"
#include "parser.h"
#include "types.h"
#include "semantic.h"
#include "ir.h"
#include "x86.h"

#include "messages.c"
#include "lexer.c"
#include "parser.c"
#include "types.c"
#include "semantic.c"
#include "ir.c"
#include "x86.c"

void
Fatal(char *Format, ...)
{
    fprintf(stderr, "fatal: ");
    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);
    fprintf(stderr, "\n");
    exit(1);
}

void
ZeroSize(memory_index Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (u8 *)Base;
    Arena->Used = 0;
}

void *
PushSize_(memory_arena *Arena, memory_index Size, memory_index Alignment)
{
    memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
    memory_index AlignmentMask = Alignment - 1;
    memory_index AlignmentOffset = 0;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }
    Assert((Arena->Used + AlignmentOffset + Size) <= Arena->Size);

    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += AlignmentOffset + Size;
    return(Result);
}

temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result = {0};
    Result.Arena = Arena;
    Result.Used = Arena->Used;
    return(Result);
}

void
EndTemporaryMemory(temporary_memory TempMemory)
{
    memory_arena *Arena = TempMemory.Arena;
    Assert(Arena->Used >= TempMemory.Used);
    Arena->Used = TempMemory.Used;
}

string
MakeString(char *Data, u32 Count)
{
    string Result = {0};
    Result.Data = Data;
    Result.Count = Count;
    return(Result);
}

string
StringFromC(char *CString)
{
    string Result = {0};
    if(CString)
    {
        Result = MakeString(CString, (u32)strlen(CString));
    }
    return(Result);
}

b32
StringsEqual(string A, string B)
{
    b32 Result = 0;
    if(A.Count == B.Count)
    {
        if(A.Count == 0)
        {
            Result = 1;
        }
        else if(A.Data && B.Data)
        {
            Result = (memcmp(A.Data, B.Data, A.Count) == 0);
        }
    }
    return(Result);
}

internal u32
GetFileSize(char *FileName, message_log *Log)
{
    u32 Result = 0;
    FILE *File = fopen(FileName, "rb");
    if(!File)
    {
        PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "cannot open '%s'", FileName);
        return(Result);
    }

    if(fseek(File, 0, SEEK_END) != 0)
    {
        PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "cannot seek '%s'", FileName);
        fclose(File);
        return(Result);
    }

    long Size = ftell(File);
    fclose(File);
    if(Size < 0)
    {
        PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "cannot size '%s'", FileName);
        return(Result);
    }

    Result = (u32)Size;
    return(Result);
}

source_file
ReadEntireFile(memory_arena *Arena, char *FileName, u32 Size, message_log *Log)
{
    FILE *File = fopen(FileName, "rb");
    if(!File)
    {
        PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "cannot open '%s'", FileName);
        return(NilSource);
    }

    char *Data = PushArray(Arena, (memory_index)Size + 1, char);
    if(fread(Data, 1, (size_t)Size, File) != (size_t)Size)
    {
        PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "cannot read '%s'", FileName);
        fclose(File);
        return(NilSource);
    }
    fclose(File);
    Data[Size] = 0;

    source_file Result = {0};
    Result.FileName = FileName;
    Result.Data = Data;
    Result.Size = Size;
    return(Result);
}

typedef struct cmdline_params
{
    char *FileName;
    char *AsmFileName;
    b32 DumpTree;
    b32 DumpIR;
    b32 ParseOnly;
    b32 Verbose;
    b32 ShowUsage;
} cmdline_params;

internal void
PrintUsage(void)
{
    printf("usage: lua [-d] [-i] [-p] [-v] [-s file.s] file.lua\n");
    printf("  -d  dump the parsed AST\n");
    printf("  -i  dump the typed IR\n");
    printf("  -p  parse and type check only; emit no assembly\n");
    printf("  -v  show timing and pass counts\n");
    printf("  -s  write assembly to file.s (default: <input>.s)\n");
}

internal cmdline_params
ParseCommandLine(int argc, char **argv, message_log *Log)
{
    cmdline_params Params = {0};
    for(int Index = 1; Index < argc; ++Index)
    {
        char *Arg = argv[Index];
        if(Arg[0] == '-')
        {
            if(strcmp(Arg, "-h") == 0)
            {
                Params.ShowUsage = 1;
            }
            else if(strcmp(Arg, "-d") == 0)
            {
                Params.DumpTree = 1;
            }
            else if(strcmp(Arg, "-i") == 0)
            {
                Params.DumpIR = 1;
            }
            else if(strcmp(Arg, "-p") == 0)
            {
                Params.ParseOnly = 1;
            }
            else if(strcmp(Arg, "-v") == 0)
            {
                Params.Verbose = 1;
            }
            else if(strcmp(Arg, "-s") == 0)
            {
                if((Index + 1) >= argc)
                {
                    PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "cmdline",
                                "-s needs a file name");
                    Params.ShowUsage = 1;
                }
                else
                {
                    Params.AsmFileName = argv[++Index];
                }
            }
            else
            {
                PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "cmdline",
                            "unknown flag '%s'", Arg);
                Params.ShowUsage = 1;
            }
        }
        else
        {
            Params.FileName = Arg;
        }
    }

    if(!Params.FileName && !Params.ShowUsage)
    {
        PushMessage(Log, Msg_Error, &NilSource, SourceOffsetNone, "cmdline",
                    "no input file");
        Params.ShowUsage = 1;
    }
    return(Params);
}

internal char *
DefaultAsmFileName(memory_arena *Arena, char *FileName)
{
    char *SearchStart = FileName;
    for(char *Scan = FileName; *Scan; ++Scan)
    {
        if((*Scan == '/') || (*Scan == '\\'))
        {
            SearchStart = Scan + 1;
        }
    }

    char *Extension = strrchr(SearchStart, '.');
    char *StemEnd = Extension ? Extension : SearchStart + strlen(SearchStart);
    u32 StemCount = (u32)(StemEnd - FileName);

    char *Result = PushArray(Arena, (memory_index)StemCount + 3, char);
    memcpy(Result, FileName, StemCount);
    memcpy(Result + StemCount, ".s", 3);
    return(Result);
}

internal void
VerbosePrint(cmdline_params Params, char *Format, ...)
{
    if(Params.Verbose)
    {
        va_list Args;
        va_start(Args, Format);
        vfprintf(stderr, Format, Args);
        va_end(Args);
    }
}

internal memory_index
ComputeArenaSize(u32 FileSize)
{
    memory_index NodeCount = (memory_index)FileSize + 16;
    memory_index Result = (memory_index)FileSize + 1;

    Result += NodeCount * (sizeof(Expr) + sizeof(Statement));
    Result += NodeCount * (sizeof(semantic_symbol) * 3 +
                           sizeof(u32) * 6 + sizeof(function_signature) * 2);
    Result += NodeCount * (sizeof(ir_instruction) * 8 + sizeof(u32) * 28 +
                           sizeof(ir_reg_def) * 12 + sizeof(b32) * 12 +
                           sizeof(ir_constant) * 4 +
                           sizeof(ir_reg_type) * 16 +
                           sizeof(ir_function_info) * 2);
    Result += NodeCount * (sizeof(asm_string) * 8 +
                           sizeof(asm_number) * 8 + sizeof(u32) * 2);
    Result += Megabytes(4);
    return(Result);
}

#define MESSAGE_ARENA_SIZE Kilobytes(64)

int
main(int argc, char **argv)
{
    int ExitCode = 1;
    void *CompileMemory = 0;
    FILE *Output = 0;
    u8 MessageMemory[MESSAGE_ARENA_SIZE];
    memory_arena LogArena = {0};
    memory_arena CompileArena = {0};
    message_log Log = {0};
    cmdline_params Params = {0};
    source_file Source = NilSource;
    Parser P = {0};
    semantic_context Semantic = {0};
    ir_program IR = {0};
    ASTIndex Root = ASTNone;
    u32 FileSize = 0;
    memory_index ArenaSize = 0;
    clock_t Start = 0;
    clock_t ParseEnd = 0;
    clock_t SemanticEnd = 0;
    clock_t GenerateEnd = 0;
    clock_t IREnd = 0;
    clock_t EmitEnd = 0;

    /* The diagnostic arena is a fixed stack buffer: expected failures are
     * appended to this log and reported at the one shallow exit below. No pass
     * prints or exits on its own, and the common path performs no allocation
     * for diagnostics at all. */
    InitializeArena(&LogArena, sizeof(MessageMemory), MessageMemory);
    InitMessageLog(&Log, &LogArena);

    Params = ParseCommandLine(argc, argv, &Log);
    if(Params.ShowUsage)
    {
        PrintUsage();
        ExitCode = MessageLogHasErrors(&Log) ? 1 : 0;
        goto done;
    }

    FileSize = GetFileSize(Params.FileName, &Log);
    if(MessageLogHasErrors(&Log))
    {
        goto done;
    }

    /* One arena for the whole compilation: source, AST, symbols, IR and the
     * assembly string tables all live here and die with the process. */
    ArenaSize = ComputeArenaSize(FileSize);
    CompileMemory = malloc(ArenaSize);
    if(!CompileMemory)
    {
        PushMessage(&Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "out of memory");
        goto done;
    }
    InitializeArena(&CompileArena, ArenaSize, CompileMemory);

    if(!Params.AsmFileName)
    {
        Params.AsmFileName = DefaultAsmFileName(&CompileArena, Params.FileName);
    }
    Source = ReadEntireFile(&CompileArena, Params.FileName, FileSize, &Log);
    if(MessageLogHasErrors(&Log))
    {
        goto done;
    }

    Start = clock();
    InitParser(&P, &CompileArena, &Source, &Log);
    Root = ParseChunk(&P);
    if(!ParserOK(&P))
    {
        /* A failed parse produces a zero-valid empty tree. Later passes can
         * still run without special "parse failed" codepaths. */
        Root = ASTNone;
    }
    ParseEnd = clock();
    VerbosePrint(Params, "parse: %u statements, %u expressions, %.3f ms\n",
                 P.StmtLen, P.ExprLen,
                 1000.0 * (f64)(ParseEnd - Start) / (f64)CLOCKS_PER_SEC);

    if(Params.DumpTree)
    {
        DumpStatement(&P, Root);
    }

    InitSemantic(&Semantic, &P, &CompileArena, &Log);
    AnalyzeSemantic(&Semantic, Root);
    SemanticEnd = clock();
    VerbosePrint(Params, "semantic: %u symbols, %u functions, %.3f ms\n",
                 Semantic.SymbolCount, Semantic.FunctionCount,
                 1000.0 * (f64)(SemanticEnd - ParseEnd) /
                     (f64)CLOCKS_PER_SEC);

    if(MessageLogHasErrors(&Log))
    {
        goto done;
    }

    if(Params.ParseOnly)
    {
        ExitCode = 0;
        goto done;
    }

    /* Check the shallow, user-visible resource boundary before entering the
     * optimizer and emitter. */
    Output = fopen(Params.AsmFileName, "w");
    if(!Output)
    {
        PushMessage(&Log, Msg_Error, &NilSource, SourceOffsetNone, "platform",
                    "cannot write '%s'", Params.AsmFileName);
        goto done;
    }

    InitIR(&IR, &P, &Semantic, &CompileArena);
    GenerateIR(&IR, Root);

    GenerateEnd = clock();
    VerbosePrint(Params, "ir: %u instructions, %u registers, %.3f ms\n",
                 IR.InstructionCount, IR.FrameRegCount,
                 1000.0 * (f64)(GenerateEnd - SemanticEnd) /
                     (f64)CLOCKS_PER_SEC);

    u32 PreOptInstructionCount = IR.InstructionCount;
    OptimizeIR(&IR);
    IREnd = clock();
    VerbosePrint(Params, "opt: %u -> %u instructions, %.3f ms\n",
                 PreOptInstructionCount, IR.InstructionCount,
                 1000.0 * (f64)(IREnd - GenerateEnd) / (f64)CLOCKS_PER_SEC);

    if(Params.DumpIR)
    {
        DumpIR(&IR);
    }

    VerbosePrint(Params, "asm: %s\n", Params.AsmFileName);
    EmitAssembly(Output, &IR, &CompileArena);
    EmitEnd = clock();
    VerbosePrint(Params, "emit: %.3f ms\n",
                 1000.0 * (f64)(EmitEnd - IREnd) / (f64)CLOCKS_PER_SEC);

    ExitCode = 0;

done:
    if(Output)
    {
        fclose(Output);
    }
    PrintMessages(&Log, stderr);
    free(CompileMemory);
    return(ExitCode);
}
