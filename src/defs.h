/* ========================================================================

   $File: defs.h
   $Date: 2026/11/20
   $Revision: 1

   NOTE: Types, macros and the arena are taken from actual Casey Muratori
   code: handmade_platform.h (u32/b32/f64, internal, ArrayCount, Assert,
   InvalidCodePath) and handmade.h (PushSize/PushStruct/PushArray, temporary
   memory). We only compile one translation unit, so these are global
   declarations rather than an API with per-file headers.

   ======================================================================== */

#ifndef LUA_DEFS_H
#define LUA_DEFS_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* C has no C++-style implicit struct tags: forward typedefs keep the
 * Casey-style bare type names (memory_arena, string, source_file). */
typedef struct memory_arena memory_arena;
typedef struct temporary_memory temporary_memory;
typedef struct string string;
typedef struct source_file source_file;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef i32 b32;
typedef float f32;
typedef double f64;
typedef size_t memory_index;

#define internal static
#define global_variable static
#define local_persist static

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(Count) ((Count) * 1024ull)
#define Megabytes(Count) (Kilobytes(Count) * 1024ull)
#define Minimum(A, B) (((A) < (B)) ? (A) : (B))
#define Maximum(A, B) (((A) > (B)) ? (A) : (B))

void Fatal(char *Format, ...);
void Error(char *Format, ...);

#define Assert(Expression)                                                     \
do                                                                         \
{                                                                          \
    if (!(Expression))                                                     \
    {                                                                      \
        Fatal("assertion failed: %s (%s:%d)", #Expression, __FILE__,       \
              __LINE__);                                                   \
    }                                                                      \
} while (0)

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase                                                     \
default:                                                                   \
    {                                                                          \
        InvalidCodePath;                                                       \
    } break

//
// One arena for the whole compile. Tokens, strings, AST nodes and variables
// are pushed here; nothing is freed until the process exits.
//

struct memory_arena
{
    memory_index Size;
    u8 *Base;
    memory_index Used;
};

void InitializeArena(memory_arena *Arena, memory_index Size, void *Base);
void *PushSize_(memory_arena *Arena, memory_index Size, memory_index Alignment);
void ZeroSize(memory_index Size, void *Ptr);

#define PushSize(Arena, Size) PushSize_(Arena, Size, 4)
#define PushArray(Arena, Count, Type)                                          \
((Type *)PushSize_(Arena, (Count) * sizeof(Type), _Alignof(Type)))
#define PushStruct(Arena, Type)                                                \
((Type *)PushSize_(Arena, sizeof(Type), _Alignof(Type)))
#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

temporary_memory BeginTemporaryMemory(memory_arena *Arena);
void EndTemporaryMemory(temporary_memory TempMemory);

//
// Strings are borrowed slices of the source file. CopySliceToCString in the
// lexer is the only thing that allocates, and it pushes into this arena.
//

struct string
{
    char *Data;
    u32 Count;
};

string MakeString(char *Data, u32 Count);
string StringFromC(char *CString);
b32 StringsEqual(string A, string B);

struct source_file
{
    char *FileName;
    char *Data;
    u32 Size;
};

source_file ReadEntireFile(memory_arena *Arena, char *FileName);

#endif /* LUA_DEFS_H */
