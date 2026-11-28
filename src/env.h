#pragma once

#include <stdbool.h>

#include "ir.h"

typedef struct
{
    const char *name;
    Literal value;
} EnvEntry;

struct Env
{
    EnvEntry *entries;
    u32 count;
    u32 capacity;
};

Env *initEnv(void);
void destroyEnv(Env *env);
Literal envGet(Env *env, const char *name);
void envSet(Env *env, const char *name, Literal value);
bool envWasSet(Env *env, const char *name);
