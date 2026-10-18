#include "allocator.h"
#include <stddef.h>

#define ARENA_DEFAULT_CAPACITY 10 * 1024 // 10KB

// Arena Allocator
Arena *init_arena(Arena *arena, ArenaRegion *region)
{
    if (!arena) return NULL;
    if (!region) return NULL;
    arena->first = region;
    arena->current = region;
    init_region(region, arena->default_capacity);
    return arena;
}

Arena *arena_create(size_t capacity)
{
    Arena *arena;
    ArenaRegion *region;

    arena = malloc(sizeof(Arena));
    if (!arena) return NULL;
    if (capacity != 0) {
        arena->default_capacity = capacity;
    } else {
        arena->default_capacity = ARENA_DEFAULT_CAPACITY;
    }

    region = malloc(sizeof(ArenaRegion));
    if (!region) return NULL;

    init_arena(arena, region);
    region->mem_chunk = malloc(region->capacity);
    if (!region->mem_chunk) return NULL;
    return arena;
}

ArenaRegion *init_region(ArenaRegion *region, size_t capacity)
{
    region->index = 0;
    region->capacity = capacity;
    region->is_free = true;
    return region;
}

void arena_clear(Arena *arena)
{
    if (!arena) return;
    ArenaRegion *walk_region = arena->first;
    while ((walk_region != NULL) && walk_region != arena->current) {
        arena_clear_region(walk_region);
        walk_region = walk_region->next_chunk;
    }
    if (arena->current) {
        arena_clear_region(arena->current);
    }
    arena->current = arena->first;
}

void arena_destroy(Arena *arena)
{
    if (!arena) return;
    ArenaRegion *walk_region = arena->first;
    while ((walk_region != NULL) && walk_region != arena->current) {
        arena_destroy_region(walk_region);
        walk_region = walk_region->next_chunk;
    }
    if (arena->current) {
        arena_destroy_region(arena->current);
    }
    free(arena);
}

void arena_clear_region(ArenaRegion *region)
{
    region->index = 0;
    region->is_free = true;
}

void arena_destroy_region(ArenaRegion *region)
{
    free(region->mem_chunk);
    free(region);
}

void *aralloc(Arena *arena, size_t size)
{
    if (!arena) return NULL;
    size_t align = _Alignof(max_align_t);
    ArenaRegion *region = arena->current;
    if (!region) return NULL;

    size_t offset = align - (size_t)(region->index + region->mem_chunk) % align;
    offset = offset == align ? 0 : offset;

    if (((region->index + offset) > region->capacity) ||
        ((region->capacity - region->index - offset) < size)) {
        region->is_free = false;
    }

    if (!region->is_free) {
        region->next_chunk = malloc(sizeof(ArenaRegion));
        if (!region->next_chunk) return NULL;

        region->next_chunk->next_chunk = NULL;
        arena->current = region->next_chunk;

        region = init_region(region->next_chunk, region->capacity);
        region->mem_chunk = malloc(region->capacity);
        if (!region->mem_chunk) return NULL;
    }

    region->index += offset;
    region->index += size;
    return (void *)(region->mem_chunk + (region->index - size));
}
