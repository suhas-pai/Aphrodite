/*
 * lib/adt/hashmap.h
 * © suhas pai
 */

#pragma once
#include <lib/list.h>

typedef void *hashmap_key_t;
#define hashmap_key_create(key) (hashmap_key_t)(uint64_t)(key)

struct hashmap_node {
    struct list list;

    hashmap_key_t key;
    char data[];
};

struct hashmap_bucket {
    struct list node_list;
};

struct hashmap;

typedef uint32_t
(*hashmap_hash_t)(hashmap_key_t key, const struct hashmap *hashmap);

uint32_t hashmap_no_hash(hashmap_key_t key, const struct hashmap *hashmap);

struct hashmap {
    struct hashmap_bucket **buckets;

    uint32_t bucket_count;
    uint32_t object_size;

    uint32_t (*hash)(hashmap_key_t key, const struct hashmap *hashmap);
    void *cb_info;
};

#define HASHMAP_INIT(obj_size, bucket_count_, hash_func, hash_cb_info) \
    ((struct hashmap){ \
        .buckets = nullptr, \
        .bucket_count = (bucket_count_), \
        .object_size = (obj_size), \
        .hash = (hash_func), \
        .cb_info = (hash_cb_info) \
    })

#define hashmap_foreach_bucket(hashmap, iter) \
    arrptr_foreach((hashmap)->buckets, (hashmap)->bucket_count, iter)

#define hashmap_bucket_foreach_node(bucket, iter) \
    struct hashmap_node *iter = nullptr; \
    list_foreach(&(bucket)->node_list, list, iter)

#define hashmap_foreach_node(hashmap, iter) \
    struct hashmap_node *iter = nullptr; \
    arrptr_foreach((hashmap)->buckets, (hashmap)->bucket_count, h_var(bucket)) \
        if (*h_var(bucket) != nullptr) \
            list_foreach(&(*h_var(bucket))->node_list, list, iter)

struct hashmap *
hashmap_alloc(uint32_t object_size,
              uint32_t bucket_count,
              hashmap_hash_t hash,
              void *cb_info);

bool
hashmap_add(struct hashmap *hashmap, hashmap_key_t key, const void *object);

bool
hashmap_update(struct hashmap *hashmap,
               hashmap_key_t key,
               const void *object,
               bool add_if_missing);

void *hashmap_get(const struct hashmap *hashmap, hashmap_key_t key);

bool hashmap_resize(struct hashmap *hashmap, uint32_t bucket_count);
bool hashmap_remove(struct hashmap *hashmap, hashmap_key_t key, void *object);

void hashmap_destroy(struct hashmap *hashmap);
void hashmap_free(struct hashmap *hashmap);
