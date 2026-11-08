#include "map.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

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

hmap *map_create()
{
    hmap *maplist = malloc(sizeof(hmap));
    if (maplist == NULL) {
        return NULL;
    }

    maplist->capacity = MAP_CAPACITY_INIT;
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

    while (maplist->index_list[index] != 0) {
        if ((maplist->length * 2) >= maplist->capacity) {
            maplist->entries = realloc(
                maplist->entries, sizeof(map_entry) * maplist->capacity * 2);

            maplist->index_list = realloc(maplist->index_list,
                                          sizeof(char) * maplist->capacity * 2);
            maplist->capacity = maplist->capacity * 2;
        }

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
        if (strcmp(maplist->entries[index].key, key) != 0) {
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
