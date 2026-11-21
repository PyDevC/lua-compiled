/* ========================================================================

   $File: types.h
   $Date: 2026/11/20
   $Revision: 1

   Strict static types for the Lua subset. There are no type annotations in
   the source language, so unknown types are inference variables: the first
   concrete use fixes them and later uses must agree.

   ======================================================================== */

#ifndef LUA_TYPES_H
#define LUA_TYPES_H

#include "defs.h"

typedef enum type_kind
{
    Type_Invalid,
    Type_Unknown,
    Type_Nil,
    Type_Bool,
    Type_Number,
    Type_String,
    Type_Function,
} type_kind;

typedef struct lua_type
{
    type_kind Kind;
    u32 SignatureIndex;
} lua_type;

internal lua_type SimpleType(type_kind Kind);
internal b32 TypeIsValid(lua_type Type);
internal b32 TypeIsUnknown(lua_type Type);
internal b32 TypesEqual(lua_type A, lua_type B);
internal b32 UnifyType(lua_type *A, lua_type B);
internal char *TypeName(lua_type Type);

#endif /* LUA_TYPES_H */
