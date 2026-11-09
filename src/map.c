#include "map.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FIBO_HASH_PRIME 11400714819323198485ULL
#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

typedef struct
{
    const char *key;
    void *data;
} map_entry;

struct hmap
{
    map_entry *entries;
    char *index_list;
    size_t capacity;
    size_t length;
};

// TODO: check what you should use size_t or u32
hmap *map_create(size_t capacity)
{
    hmap *maplist = malloc(sizeof(hmap));
    if (maplist == NULL) {
        return NULL;
    }

    maplist->capacity = capacity < 1 ? MAP_CAPACITY_INIT : capacity;
    maplist->length = 0;

    maplist->entries = malloc(sizeof(map_entry) * maplist->capacity);
    maplist->index_list = malloc(sizeof(char) * maplist->capacity);

    if (maplist->entries == NULL) {
        return NULL;
    }

    return maplist;
}

void map_destroy(hmap *maplist)
{
    free(maplist->entries);
    free(maplist->index_list);
    free(maplist);
}

size_t fibo_hash_key(size_t hash) { return (hash * FIBO_HASH_PRIME) >> 11; }

size_t hash_key(const char *key)
{
    size_t hash = FNV_OFFSET;
    size_t prime = FIBO_HASH_PRIME;

    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= prime;
    }
    return hash;
}

hmap *map_resize(hmap *maplist, size_t new_capacity)
{
    map_entry *new_entries = calloc(new_capacity, sizeof *new_entries);
    char *new_index_list = calloc(new_capacity, sizeof *new_index_list);

    for (int i = 0; i < maplist->capacity; ++i) {
        if (maplist->index_list[i] == 0) {
            continue;
        }

        const char *key = maplist->entries[i].key;
        void *value = maplist->entries[i].data;

        size_t hash = hash_key(key);
        size_t index = (size_t)(fibo_hash_key(hash) & (new_capacity - 1));
        while (new_index_list[index] != 0) {
            index++;
            index = index % new_capacity;
        }

        new_entries[index].data = value;
        new_entries[index].key = key;
        new_index_list[index] = 1;
    }

    free(maplist->entries);
    free(maplist->index_list);
    maplist->entries = new_entries;
    maplist->index_list = new_index_list;
    maplist->capacity = new_capacity;
    return maplist;
}

const char *map_set_entry(hmap *maplist, const char *key, void *value)
{
    if ((maplist->length * 2) >= maplist->capacity) {
        maplist = map_resize(maplist, maplist->capacity * 2);
    }

    size_t hash = hash_key(key);
    size_t index = (size_t)(fibo_hash_key(hash) & (maplist->capacity - 1));

    while (maplist->index_list[index] != 0) {
        index++;
        index = index % maplist->capacity;
    }

    maplist->entries[index].data = value;
    maplist->entries[index].key = key;
    maplist->index_list[index] = 1;
    maplist->length++;
    return key;
}

void *map_get_value(hmap *maplist, const char *key)
{
    size_t hash = hash_key(key);
    size_t index = (size_t)(fibo_hash_key(hash) & (maplist->capacity - 1));

    while (maplist->index_list[index] == 1) {
        if (strcmp(maplist->entries[index].key, key) == 0) {
            return maplist->entries[index].data;
        }
        index = (index + 1) % maplist->capacity;
    }

    return NULL;
}

void *map_delete_entry(hmap *maplist, const char *key)
{
    size_t hash = hash_key(key);
    size_t index = (size_t)(fibo_hash_key(hash) & (maplist->capacity - 1));

    while (maplist->index_list[index] == 1) {
        if (strcmp(maplist->entries[index].key, key) != 0) {
            maplist->index_list = 0;
        }
        index = (index + 1) % maplist->capacity;
    }

    return NULL;
}

size_t map_size(hmap *maplist) { return maplist->length; }
