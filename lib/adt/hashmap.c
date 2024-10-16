/*
 * lib/adt/hashmap.c
 * © suhas pai
 */

#include "lib/alloc.h"
#include "lib/list.h"

#include "hashmap.h"

__debug_optimize(3) uint32_t
hashmap_no_hash(const hashmap_key_t key, const struct hashmap *const hashmap) {
    (void)hashmap;
    return (uint32_t)(uint64_t)key;
}

struct hashmap *
hashmap_alloc(const uint32_t object_size,
              const uint32_t bucket_count,
              const hashmap_hash_t hash,
              void *const cb_info)
{
    assert_msg(hash != NULL, "hashmap_init(): hash function cannot be NULL");

    struct hashmap *const hashmap = malloc(sizeof(*hashmap));
    if (hashmap == NULL) {
        return NULL;
    }

    *hashmap = HASHMAP_INIT(object_size, bucket_count, hash, cb_info);
    return hashmap;
}

__debug_optimize(3)
static inline bool hashmap_initialized(const struct hashmap *const hashmap) {
    return hashmap->bucket_count != 0 && hashmap->object_size != 0;
}

__debug_optimize(3) static inline
uint32_t hash_of(const struct hashmap *const hashmap, const hashmap_key_t key) {
    return hashmap->hash(key, hashmap) % hashmap->bucket_count;
}

bool
hashmap_add(struct hashmap *const hashmap,
            const hashmap_key_t key,
            const void *const object)
{
    assert_msg(hashmap_initialized(hashmap),
               "hashmap_add(): hashmap not initialized");

    bool alloced_buckets = false;
    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        hashmap->buckets =
            calloc_size(hashmap->bucket_count,
                        sizeof(struct hashmap_bucket *),
                        &hashmap->bucket_count);

        if (hashmap->buckets == NULL) {
            return false;
        }

        alloced_buckets = true;
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *bucket = hashmap->buckets[key_hash];

    if (bucket != NULL) {
        hashmap_bucket_foreach_node(bucket, iter) {
            if (iter->key != key) {
                continue;
            }

            if (list_empty(&bucket->node_list)) {
                hashmap->buckets[key_hash] = NULL;
                free(bucket);
            }

            if (__builtin_expect(alloced_buckets, 0)) {
                free(hashmap->buckets);
                hashmap->buckets = NULL;
            }

            return false;
        }
    } else {
        bucket = calloc(1, sizeof(struct hashmap_bucket));
        if (bucket == NULL) {
            if (__builtin_expect(alloced_buckets, 0)) {
                free(hashmap->buckets);
                hashmap->buckets = NULL;
            }

            return false;
        }

        list_init(&bucket->node_list);
        hashmap->buckets[key_hash] = bucket;
    }

    struct hashmap_node *const node =
        calloc(1, sizeof(*node) + hashmap->object_size);

    if (node == NULL) {
        if (list_empty(&bucket->node_list)) {
            hashmap->buckets[key_hash] = NULL;
            free(bucket);
        }

        if (__builtin_expect(alloced_buckets, 0)) {
            free(hashmap->buckets);
            hashmap->buckets = NULL;
        }

        return false;
    }

    node->key = key;

    __builtin_memcpy(node->data, object, hashmap->object_size);
    list_add(&bucket->node_list, &node->list);

    return true;
}

bool
hashmap_update(struct hashmap *const hashmap,
               const hashmap_key_t key,
               const void *const object,
               const bool add_if_missing)
{
    assert_msg(hashmap_initialized(hashmap),
               "hashmap_update(): hashmap not initialized");

    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        if (add_if_missing) {
            return hashmap_add(hashmap, key, object);
        }

        return false;
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *bucket = hashmap->buckets[key_hash];

    if (bucket == NULL) {
        if (!add_if_missing) {
            return false;
        }

        bucket = calloc(1, sizeof(struct hashmap_bucket));
        if (bucket == NULL) {
            return false;
        }

        list_init(&bucket->node_list);
        hashmap->buckets[key_hash] = bucket;
    } else {
        hashmap_bucket_foreach_node(bucket, iter) {
            if (iter->key == key) {
                __builtin_memcpy(iter->data, object, hashmap->object_size);
                return true;
            }
        }

        if (!add_if_missing) {
            return false;
        }
    }

    struct hashmap_node *const node =
        calloc(1, sizeof(*node) + hashmap->object_size);

    if (node == NULL) {
        if (list_empty(&bucket->node_list)) {
            hashmap->buckets[key_hash] = NULL;
            free(bucket);
        }

        return false;
    }

    node->key = key;

    __builtin_memcpy(node->data, object, hashmap->object_size);
    list_add(&bucket->node_list, &node->list);

    return true;
}

void *
hashmap_get(const struct hashmap *const hashmap, const hashmap_key_t key) {
    assert_msg(hashmap_initialized(hashmap),
               "hashmap_get(): hashmap not initialized");

    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        return NULL;
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *const bucket = hashmap->buckets[key_hash];

    if (bucket == NULL) {
        return NULL;
    }

    hashmap_bucket_foreach_node(bucket, iter) {
        if (iter->key == key) {
            return iter->data;
        }
    }

    return NULL;
}

bool
hashmap_remove(struct hashmap *const hashmap,
               const hashmap_key_t key,
               void *const object_ptr)
{
    assert_msg(hashmap_initialized(hashmap),
               "hashmap_remove(): hashmap not initialized");

    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        return NULL;
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *const bucket = hashmap->buckets[key_hash];

    if (bucket == NULL) {
        return NULL;
    }

    struct hashmap_node *iter = NULL;
    struct hashmap_node *tmp = NULL;

    list_foreach_mut(iter, tmp, &bucket->node_list, list) {
        if (iter->key != key) {
            continue;
        }

        list_deinit(&iter->list);
        if (list_empty(&bucket->node_list)) {
            hashmap->buckets[key_hash] = NULL;
            free(bucket);
        }

        if (object_ptr != NULL) {
            __builtin_memcpy(object_ptr, iter->data, hashmap->object_size);
        }

        free(iter);
        return true;
    }

    return false;
}

static void
destroy_hashmap_buckets(struct hashmap_bucket **const buckets,
                        const uint32_t bucket_count)
{
    struct hashmap_bucket **const end = buckets + bucket_count;
    for (__auto_type iter = buckets; iter != end; iter++) {
        struct hashmap_bucket *const bucket = *iter;
        if (bucket == NULL) {
            continue;
        }

        struct hashmap_node *node = NULL;
        struct hashmap_node *tmp = NULL;

        list_foreach_mut(node, tmp, &bucket->node_list, list) {
            free(node);
        }

        free(bucket);
    }

    free(buckets);
}

bool
hashmap_resize(struct hashmap *const hashmap, const uint32_t bucket_count) {
    assert_msg(hashmap_initialized(hashmap),
               "hashmap_resize(): hashmap not initialized");
    assert_msg(bucket_count != 0,
               "hashmap_resize(): trying to resize to bucket-count of zero, "
               "use hashmap_destroy() instead");

    // Allow downsizing the hashmap
    if (hashmap->bucket_count == bucket_count) {
        return true;
    }

    struct hashmap_bucket **const buckets =
        calloc(bucket_count, sizeof(struct hashmap_bucket *));

    if (buckets == NULL) {
        return false;
    }

    struct hashmap_bucket **old_buckets = hashmap->buckets;
    uint32_t old_bucket_count = hashmap->bucket_count;

    hashmap->buckets = buckets;
    hashmap->bucket_count = bucket_count;

    struct hashmap_bucket *const *const end = old_buckets + old_bucket_count;
    for (struct hashmap_bucket **iter = old_buckets; iter != end; iter++) {
        struct hashmap_bucket *const bucket = *iter;
        if (bucket == NULL) {
            continue;
        }

        hashmap_bucket_foreach_node(bucket, jter) {
            if (!hashmap_add(hashmap, jter->key, jter->data)) {
                hashmap_destroy(hashmap);

                hashmap->buckets = old_buckets;
                hashmap->bucket_count = old_bucket_count;

                return false;
            }
        }
    }

    if (old_buckets != NULL) {
        destroy_hashmap_buckets(old_buckets, old_bucket_count);
    }

    return true;
}

void hashmap_destroy(struct hashmap *const hashmap) {
    destroy_hashmap_buckets(hashmap->buckets, hashmap->bucket_count);

    hashmap->buckets = NULL;
    hashmap->bucket_count = 0;

    hashmap->hash = NULL;
    hashmap->object_size = 0;
    hashmap->cb_info = NULL;
}

void hashmap_free(struct hashmap *const hashmap) {
    hashmap_destroy(hashmap);
    free(hashmap);
}