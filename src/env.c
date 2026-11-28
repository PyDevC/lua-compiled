#include "env.h"

#include <stdlib.h>
#include <string.h>

Env *initEnv(void)
{
    Env *env = calloc(1, sizeof(Env));
    return env;
}

void destroyEnv(Env *env)
{
    if (env == NULL) {
        return;
    }

    for (u32 i = 0; i < env->count; ++i) {
        free((void *)env->entries[i].name);
    }
    free(env->entries);
    free(env);
}

internal void envGrow(Env *env)
{
    if (env->count < env->capacity) {
        return;
    }

    env->capacity = env->capacity == 0 ? 8u : env->capacity * 2u;
    EnvEntry *new_entries =
        realloc(env->entries, env->capacity * sizeof(EnvEntry));
    if (new_entries == NULL) {
        fprintf(stderr, "env: out of memory\n");
        abort();
    }
    env->entries = new_entries;
}

Literal envGet(Env *env, const char *name)
{
    for (u32 i = 0; i < env->count; ++i) {
        if (strcmp(env->entries[i].name, name) == 0) {
            return env->entries[i].value;
        }
    }
    return 0.0;
}

void envSet(Env *env, const char *name, Literal value)
{
    for (u32 i = 0; i < env->count; ++i) {
        if (strcmp(env->entries[i].name, name) == 0) {
            env->entries[i].value = value;
            return;
        }
    }

    envGrow(env);
    size_t name_length = strlen(name);
    char *name_copy = malloc(name_length + 1);
    if (name_copy == NULL) {
        fprintf(stderr, "env: out of memory\n");
        abort();
    }
    memcpy(name_copy, name, name_length + 1);

    env->entries[env->count].name = name_copy;
    env->entries[env->count].value = value;
    env->count++;
}

bool envWasSet(Env *env, const char *name)
{
    for (u32 i = 0; i < env->count; ++i) {
        if (strcmp(env->entries[i].name, name) == 0) {
            return true;
        }
    }
    return false;
}
