#pragma once

#include <stddef.h>

#define MAP_CAPACITY_INIT 8

typedef struct hmap hmap;

hmap *map_create(size_t capacity);
void map_destroy(hmap *maplist);

/* Copies key. Returns stored key copy, or NULL on allocation failure.
 * Existing key: value replaced. */
const char *map_set_entry(hmap *maplist, const char *key, void *value);

/* Returns value or NULL if absent. */
void *map_get_value(hmap *maplist, const char *key);

/* Returns removed value or NULL if absent. */
void *map_delete_entry(hmap *maplist, const char *key);

size_t map_size(hmap *maplist);
