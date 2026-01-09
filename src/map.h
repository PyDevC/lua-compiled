#ifndef MAP_H
#define MAP_H
#include <limits.h>

#include <stdbool.h>
#include <stdio.h>

/**
 **/

#define MAP_CAPACITY_INIT 117148193
#define FIBO_HASH_PRIME 11400714819323198485LLU
#define FIBO_OFFSET 0xcbf29ce484222325LLU
#define FIBO_OFFSET_MINI 11;

typedef struct hmap hmap;

hmap *map_create();
void map_destroy(hmap *maplist);
void *map_get_value(hmap *maplist, const char *key);
const char *map_set_entry(hmap *maplist, const char *key, void *value);
void *map_delete_entry(hmap *maplist, const char *key);
size_t map_size(hmap *maplist);

/* Hash Iterators */
typedef struct
{
    const char *key;
    void *value;
    hmap *_table;
    size_t _index;
} hmap_itr;

hmap_itr hmap_itr_create(hmap *maplist);
bool hmap_itr_next(hmap_itr *iter);

/* Fibonacci hashing function */
size_t fibo_hash_key(size_t hash);
size_t hash_key(const char *key);

#endif // MAP_H
