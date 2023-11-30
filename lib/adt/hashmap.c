/*
 * lib/adt/hashmap.c
 * © suhas pai
 */

#include "lib/alloc.h"
#include "lib/list.h"
#include "lib/string.h"

#include "hashmap.h"

bool hashmap_alloc(struct hashmap *const hashmap, const uint32_t bucket_count) {
    hashmap->buckets = malloc(sizeof(struct hashmap_bucket *) * bucket_count);
    if (hashmap->buckets == NULL) {
        return false;
    }

    hashmap->bucket_count = bucket_count;
    return true;
}

__optimize(3)
static inline uint32_t hash_of(struct hashmap *const hashmap, void *const key) {
    return hashmap->hash(key, hashmap) % hashmap->bucket_count;
}

bool
hashmap_add(struct hashmap *const hashmap, void *const key, void *const object)
{
    assert_msg(hashmap->bucket_count != 0 && hashmap->object_size != 0,
               "hashmap_add(): hashmap not initialized");

    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        hashmap->buckets =
            malloc(sizeof(struct hashmap_bucket *) * hashmap->bucket_count);

        if (hashmap->buckets == NULL) {
            return false;
        }
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *bucket = hashmap->buckets[key_hash];

    if (bucket == NULL) {
        bucket = malloc(sizeof(struct hashmap_bucket));
        if (bucket == NULL) {
            return false;
        }

        list_init(&bucket->node_list);
        hashmap->buckets[key_hash] = bucket;
    }

    struct hashmap_node *const node =
        malloc(sizeof(*node) + hashmap->object_size);

    if (node == NULL) {
        if (list_empty(&bucket->node_list)) {
            hashmap->buckets[key_hash] = NULL;
            free(bucket);
        }

        return false;
    }

    node->key = key;

    memcpy(node->data, object, hashmap->object_size);
    list_add(&bucket->node_list, &node->list);

    return true;
}

void *hashmap_get(struct hashmap *const hashmap, void *const key) {
    assert_msg(hashmap->bucket_count != 0 && hashmap->object_size != 0,
               "hashmap_get(): hashmap not initialized");

    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        return false;
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *const bucket = hashmap->buckets[key_hash];

    if (bucket == NULL) {
        return NULL;
    }

    struct hashmap_node *iter = NULL;
    list_foreach(iter, &bucket->node_list, list) {
        if (iter->key == key) {
            return iter->data;
        }
    }

    return NULL;
}

bool
hashmap_remove(struct hashmap *const hashmap,
               void *const key,
               void *const object_ptr)
{
    assert_msg(hashmap->bucket_count != 0 && hashmap->object_size != 0,
               "hashmap_remove(): hashmap not initialized");

    if (__builtin_expect(hashmap->buckets == NULL, 0)) {
        return false;
    }

    const uint32_t key_hash = hash_of(hashmap, key);
    struct hashmap_bucket *const bucket = hashmap->buckets[key_hash];

    if (bucket == NULL) {
        return NULL;
    }

    struct hashmap_node *iter = NULL;
    struct list *prev = &bucket->node_list;

    list_foreach(iter, &bucket->node_list, list) {
        if (iter->key == key) {
            prev->next = iter->list.next;
            if (list_empty(&bucket->node_list)) {
                hashmap->buckets[key_hash] = NULL;
                free(bucket);
            }

            if (object_ptr != NULL) {
                memcpy(object_ptr, iter->data, hashmap->object_size);
            }

            return iter;
        }

        prev = iter->list.next;
    }

    return false;
}

bool
hashmap_resize(struct hashmap *const hashmap, const uint32_t bucket_count) {
    assert_msg(hashmap->bucket_count != 0 && hashmap->object_size != 0,
               "hashmap_resize(): hashmap not initialized");
    assert_msg(bucket_count != 0,
               "hashmap_resize(): trying to resize with bucket-count of zero, "
               "use hashmap_destroy() instead");

    struct hashmap_bucket **const buckets =
        malloc(sizeof(struct hashmap_bucket *) * bucket_count);

    if (buckets == NULL) {
        return false;
    }

    struct hashmap_bucket **const old_buckets = hashmap->buckets;
    const uint32_t old_bucket_count = hashmap->bucket_count;

    hashmap->buckets = buckets;
    hashmap->bucket_count = bucket_count;

    struct hashmap_bucket *const *const end = old_buckets + old_bucket_count;
    for (struct hashmap_bucket **iter = old_buckets; iter != end; iter++) {
        struct hashmap_bucket *const bucket = *iter;
        if (bucket == NULL) {
            continue;
        }

        struct hashmap_node *jter = NULL;
        list_foreach(jter, &bucket->node_list, list) {
            if (!hashmap_add(hashmap, jter->key, jter->data)) {
                hashmap_destroy(hashmap);

                hashmap->buckets = old_buckets;
                hashmap->bucket_count = old_bucket_count;

                return false;
            }
        }

        free(bucket);
    }

    if (__builtin_expect(old_buckets != NULL, 1)) {
        free(old_buckets);
    }

    return true;
}

void hashmap_destroy(struct hashmap *const hashmap) {
    struct hashmap_bucket *const *const end =
        hashmap->buckets + hashmap->bucket_count;

    for (struct hashmap_bucket **iter = hashmap->buckets;
         iter != end;
         iter++)
    {
        struct hashmap_bucket *const bucket = *iter;
        if (bucket == NULL) {
            continue;
        }

        struct hashmap_node *jter = NULL;
        struct hashmap_node *tmp = NULL;

        list_foreach_mut(jter, tmp, &bucket->node_list, list) {
            free(jter);
        }

        free(bucket);
    }

    free(hashmap->buckets);
    hashmap->bucket_count = 0;
}