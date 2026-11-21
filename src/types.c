/* ========================================================================

   $File: types.c
   $Date: 2026/11/20
   $Revision: 1

   Tiny type lattice. Unknown is an inference variable; Invalid is only used
   before the semantic pass has visited a node.

   ======================================================================== */

#include "types.h"

internal lua_type
SimpleType(type_kind Kind)
{
    lua_type Result = {0};
    Result.Kind = Kind;
    return(Result);
}

internal b32
TypeIsValid(lua_type Type)
{
    b32 Result = (Type.Kind != Type_Invalid);
    return(Result);
}

internal b32
TypeIsUnknown(lua_type Type)
{
    b32 Result = (Type.Kind == Type_Unknown);
    return(Result);
}

internal b32
TypesEqual(lua_type A, lua_type B)
{
    b32 Result = 0;
    if(A.Kind == B.Kind)
    {
        if(A.Kind == Type_Function)
        {
            Result = (A.SignatureIndex == B.SignatureIndex);
        }
        else
    {
            Result = 1;
        }
    }
    return(Result);
}

internal b32
UnifyType(lua_type *A, lua_type B)
{
    b32 Result = 1;
    if(!TypeIsValid(*A))
    {
        *A = B;
    }
    else if(TypeIsUnknown(*A) && !TypeIsUnknown(B))
    {
        *A = B;
    }
    else if(!TypeIsUnknown(B) && !TypesEqual(*A, B))
    {
        Result = 0;
    }
    return(Result);
}

internal char *
TypeName(lua_type Type)
{
    char *Result = "invalid";
    switch(Type.Kind)
    {
        case Type_Invalid:  { Result = "invalid"; } break;
        case Type_Unknown:  { Result = "unknown"; } break;
        case Type_Nil:      { Result = "nil"; } break;
        case Type_Bool:     { Result = "boolean"; } break;
        case Type_Number:   { Result = "number"; } break;
        case Type_String:   { Result = "string"; } break;
        case Type_Function: { Result = "function"; } break;

            InvalidDefaultCase;
    }
    return(Result);
}
