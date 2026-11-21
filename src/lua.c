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
Error(char *Format, ...)
{
    fprintf(stderr, "error: ");
    va_list Args;
    va_start(Args, Format);
    vfprintf(stderr, Format, Args);
    va_end(Args);
    fprintf(stderr, "\n");
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
    string Result = MakeString(CString, (u32)strlen(CString));
    return(Result);
}

b32
StringsEqual(string A, string B)
{
    b32 Result = ((A.Count == B.Count) &&
                  (memcmp(A.Data, B.Data, A.Count) == 0));
    return(Result);
}

internal u32
GetFileSize(char *FileName)
{
    FILE *File = fopen(FileName, "rb");
    if(!File)
    {
        Fatal("cannot open '%s'", FileName);
    }

    if((fseek(File, 0, SEEK_END) != 0))
    {
        Fatal("cannot seek '%s'", FileName);
    }

    long Size = ftell(File);
    fclose(File);
    if(Size < 0)
    {
        Fatal("cannot size '%s'", FileName);
    }

    u32 Result = (u32)Size;
    return(Result);
}

source_file
ReadEntireFile(memory_arena *Arena, char *FileName)
{
    source_file Result = {0};
    FILE *File = fopen(FileName, "rb");
    if(!File)
    {
        Fatal("cannot open '%s'", FileName);
    }

    if(fseek(File, 0, SEEK_END) != 0)
    {
        Fatal("cannot seek '%s'", FileName);
    }
    long FileSize = ftell(File);
    if((FileSize < 0) || (fseek(File, 0, SEEK_SET) != 0))
    {
        Fatal("cannot size '%s'", FileName);
    }

    u32 Size = (u32)FileSize;
    char *Data = PushArray(Arena, (memory_index)Size + 1, char);
    if(fread(Data, 1, (size_t)Size, File) != (size_t)Size)
    {
        Fatal("cannot read '%s'", FileName);
    }
    fclose(File);
    Data[Size] = 0;

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
ParseCommandLine(int argc, char **argv)
{
    cmdline_params Params = {0};
    for(int Index = 1; Index < argc; ++Index)
    {
        char *Arg = argv[Index];
        if(Arg[0] == '-')
        {
            if(strcmp(Arg, "-h") == 0)
            {
                PrintUsage();
                exit(0);
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
                    Fatal("-s needs a file name");
                }
                Params.AsmFileName = argv[++Index];
            }
            else
            {
                fprintf(stderr, "unknown flag '%s'\n", Arg);
                PrintUsage();
                exit(1);
            }
        }
        else
        {
            Params.FileName = Arg;
        }
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

int
main(int argc, char **argv)
{
    cmdline_params Params = ParseCommandLine(argc, argv);
    if(!Params.FileName)
    {
        PrintUsage();
        return(1);
    }

    /* One arena for the whole compilation: source, AST, symbols, IR and the
     * assembly string tables all live here and die with the process. */
    u32 FileSize = GetFileSize(Params.FileName);
    memory_index ArenaSize = ComputeArenaSize(FileSize);
    void *Memory = malloc(ArenaSize);
    if(!Memory)
    {
        Fatal("out of memory");
    }

    memory_arena Arena = {0};
    InitializeArena(&Arena, ArenaSize, Memory);
    if(!Params.AsmFileName)
    {
        Params.AsmFileName = DefaultAsmFileName(&Arena, Params.FileName);
    }
    source_file Source = ReadEntireFile(&Arena, Params.FileName);

    clock_t Start = clock();

    Parser P;
    InitParser(&P, &Arena, &Source);
    ASTIndex Root = ParseChunk(&P);
    if(!ParserOK(&P))
    {
        free(Memory);
        return(1);
    }

    clock_t ParseEnd = clock();
    VerbosePrint(Params, "parse: %u statements, %u expressions, %.3f ms\n",
                 P.StmtLen, P.ExprLen,
                 1000.0 * (f64)(ParseEnd - Start) / (f64)CLOCKS_PER_SEC);

    if(Params.DumpTree)
    {
        DumpStatement(&P, Root);
    }

    semantic_context Semantic;
    InitSemantic(&Semantic, &P, &Arena);
    AnalyzeSemantic(&Semantic, Root);
    if(Semantic.HadError)
    {
        free(Memory);
        return(1);
    }

    clock_t SemanticEnd = clock();
    VerbosePrint(Params, "semantic: %u symbols, %u functions, %.3f ms\n",
                 Semantic.SymbolCount, Semantic.FunctionCount,
                 1000.0 * (f64)(SemanticEnd - ParseEnd) /
                     (f64)CLOCKS_PER_SEC);

    if(Params.ParseOnly)
    {
        free(Memory);
        return(0);
    }

    ir_program IR;
    InitIR(&IR, &P, &Semantic, &Arena);
    GenerateIR(&IR, Root);

    clock_t GenerateEnd = clock();
    VerbosePrint(Params, "ir: %u instructions, %u registers, %.3f ms\n",
                 IR.InstructionCount, IR.FrameRegCount,
                 1000.0 * (f64)(GenerateEnd - SemanticEnd) /
                     (f64)CLOCKS_PER_SEC);

    u32 PreOptInstructionCount = IR.InstructionCount;
    OptimizeIR(&IR);
    clock_t IREnd = clock();
    VerbosePrint(Params, "opt: %u -> %u instructions, %.3f ms\n",
                 PreOptInstructionCount, IR.InstructionCount,
                 1000.0 * (f64)(IREnd - GenerateEnd) / (f64)CLOCKS_PER_SEC);

    if(Params.DumpIR)
    {
        DumpIR(&IR);
    }

    FILE *Output = fopen(Params.AsmFileName, "w");
    if(!Output)
    {
        Fatal("cannot write '%s'", Params.AsmFileName);
    }

    VerbosePrint(Params, "asm: %s\n", Params.AsmFileName);
    EmitAssembly(Output, &IR, &Arena);
    fclose(Output);

    clock_t EmitEnd = clock();
    VerbosePrint(Params, "emit: %.3f ms\n",
                 1000.0 * (f64)(EmitEnd - IREnd) / (f64)CLOCKS_PER_SEC);

    free(Memory);
    return(0);
}
