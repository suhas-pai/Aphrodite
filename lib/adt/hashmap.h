/*
 * lib/adt/hashmap.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/list.h"

struct hashmap_node {
    struct list list;

    void *key;
    char data[];
};

struct hashmap_bucket {
    struct list node_list;
};

struct hashmap {
    struct hashmap_bucket **buckets;

    uint32_t bucket_count;
    uint32_t object_size;

    uint32_t (*hash)(void *key, struct hashmap *hashmap);
    void *cb_info;
};

#define HASHMAP_INIT(obj_size, bucket_count_, hash_func, hash_cb_info) \
    ((struct hashmap){ \
        .buckets = NULL, \
        .bucket_count = (bucket_count_), \
        .object_size = (obj_size), \
        .hash = (hash_func), \
        .cb_info = (hash_cb_info) \
    })

bool hashmap_add(struct hashmap *hashmap, void *key, const void *object);

void *hashmap_get(struct hashmap *hashmap, void *key);
bool hashmap_remove(struct hashmap *hashmap, void *key, void *object_ptr);

bool hashmap_resize(struct hashmap *hashmap, uint32_t bucket_count);
void hashmap_destroy(struct hashmap *hashmap);