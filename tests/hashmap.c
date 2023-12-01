/*
 * tests/hashmap.h
 * © suhas pai
 */

#include "lib/adt/hashmap.h"

uint32_t hasher(void *const key, const struct hashmap *const hashmap) {
    (void)hashmap;
    return (uint64_t)key;
}

void test_add_and_get(struct hashmap *const hashmap, int object) {
    assert(hashmap_add(hashmap, hashmap_key_create(object), &object));
    assert(!hashmap_add(hashmap, hashmap_key_create(object), &object));

    void *hm_object = hashmap_get(hashmap, hashmap_key_create(object));

    assert(hm_object != NULL);
    assert(*(int *)hm_object == object);

    const int new_ob = object + 1;
    assert(hashmap_update(hashmap, hashmap_key_create(object), &new_ob, false));

    hm_object = hashmap_get(hashmap, hashmap_key_create(object));

    assert(hm_object != NULL);
    assert(*(int *)hm_object == new_ob);

    assert(hashmap_update(hashmap, hashmap_key_create(object), &object, false));

    hm_object = hashmap_get(hashmap, hashmap_key_create(object));

    assert(hm_object != NULL);
    assert(*(int *)hm_object == object);
}

void test_get(struct hashmap *const hashmap, const int object) {
    void *const hm_object = hashmap_get(hashmap, (void *)(uint64_t)object);

    assert(hm_object != NULL);
    assert(*(int *)hm_object == object);
}

void test_remove(struct hashmap *const hashmap, int object) {
    int a = 0;
    assert(hashmap_remove(hashmap, (void *)(uint64_t)object, &a));
    assert(a == object);
}

void test_hashmap() {
    struct hashmap hashmap =
        HASHMAP_INIT(sizeof(int), /*bucket_count=*/5, hasher, NULL);

    test_add_and_get(&hashmap, /*object=*/5);
    test_add_and_get(&hashmap, /*object=*/2);
    test_add_and_get(&hashmap, /*object=*/9);
    test_add_and_get(&hashmap, /*object=*/4);

    assert(hashmap_resize(&hashmap, /*bucket_count=*/10));

    test_get(&hashmap, /*object=*/5);
    test_get(&hashmap, /*object=*/9);
    test_get(&hashmap, /*object=*/2);
    test_get(&hashmap, /*object=*/4);

    test_remove(&hashmap, /*object=*/2);
    test_remove(&hashmap, /*object=*/4);
    test_remove(&hashmap, /*object=*/5);
    test_remove(&hashmap, /*object=*/9);

    hashmap_destroy(&hashmap);
}