#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct ArenaRegion
{
    size_t capacity;                       /* capacity of memory chunk */
    size_t index;                          /* index relation */
    bool is_free;                          /* is free to get overwrite */
    struct ArenaRegion *next_chunk;        /* pointer to next memory chunk */
    _Alignas(max_align_t) char *mem_chunk; /* memory chunk of the region */
} ArenaRegion;

typedef struct Arena
{
    ArenaRegion *first;
    ArenaRegion *current;
    size_t default_capacity;
} Arena;

/* arena helpers */
Arena *init_arena(Arena *arena, ArenaRegion *region);
Arena *arena_create(size_t capacity);
void arena_clear(Arena *arena);
void arena_destroy(Arena *arena);

ArenaRegion *init_region(ArenaRegion *region, size_t capacity);
void arena_clear_region(ArenaRegion *region);
void arena_destroy_region(ArenaRegion *region);

/* allocation API */
void *aralloc(Arena *arena, size_t size);
