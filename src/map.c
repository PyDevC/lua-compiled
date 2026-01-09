#include "map.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TOMBSTONE "TOMB"

typedef struct
{
    const char *key;
    void *data;
} map_entry;

struct hmap
{
    map_entry *entries;
    size_t capacity;
    size_t length;
};

hmap *map_create()
{
    hmap *maplist = malloc(sizeof(hmap));
    if (maplist == NULL) {
        printf("maplist is null\n");
        return NULL;
    }

    maplist->capacity = MAP_CAPACITY_INIT;
    maplist->length = 0;

    maplist->entries = calloc(maplist->capacity, sizeof(map_entry));
    if (maplist->entries == NULL) {
        printf("maplist entires is null\n");
        free(maplist);
        return NULL;
    }

    return maplist;
}

void map_destroy(hmap *maplist)
{
    size_t i;
    for (i = 0; i < maplist->length; i++) {
        free((void *)maplist->entries[i].key);
    }
    free(maplist->entries);
    free(maplist);
}

size_t fibo_hash_key(size_t hash)
{
    return (hash * FIBO_HASH_PRIME) >> FIBO_OFFSET_MINI;
}

size_t hash_key(const char *key)
{
    size_t hash = FIBO_OFFSET;
    size_t prime = FIBO_HASH_PRIME;

    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= prime;
    }
    return hash;
}

const char *map_set_entry(hmap *maplist, const char *key, void *value)
{
    size_t hash = hash_key(key);
    size_t index = (size_t)(fibo_hash_key(hash) & (maplist->capacity - 1));

    while (maplist->entries[index].key != NULL) {
        if (strcmp(maplist->entries[index].key, key) == 0) {
            maplist->entries[index].data = value;
            maplist->length++;
            return key;
        }

        index++;
        if (index >= maplist->capacity) {
            index = 0;
        }
    }

    maplist->entries[index].key = strdup(key);
    maplist->entries[index].data = value;
    maplist->length++;
    return key;
}

void *map_get_value(hmap *maplist, const char *key)
{
    size_t hash = hash_key(key);
    size_t index = (size_t)(fibo_hash_key(hash) & (maplist->capacity - 1));

    while (maplist->entries[index].key != NULL) {
        if ((strcmp(maplist->entries[index].key, TOMBSTONE) == 0) &&
            (strcmp(key, maplist->entries[index].key) == 0)) {
            return maplist->entries[index].data;
        }
        index++;
        if (index >= maplist->capacity) {
            index = 0;
        }
    }

    return NULL;
}

void *map_delete_entry(hmap *maplist, const char *key)
{
    size_t hash = hash_key(key);
    size_t index = (size_t)(fibo_hash_key(hash) & (maplist->capacity - 1));

    while (maplist->entries[index].key != NULL) {
        if (strcmp(key, maplist->entries[index].key) == 0) {
            void *data = maplist->entries[index].data;

            maplist->entries[index].key = TOMBSTONE;
            maplist->entries[index].data = NULL;

            maplist->length++;
            return data;
        }
        index++;
        if (index >= maplist->capacity) {
            index = 0;
        }
    }

    return NULL;
}

size_t map_size(hmap *maplist) { return maplist->length; }
