/*
 * kernel/src/dev/storage/nvme/cache.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "sched/thread.h"

#include "cache.h"

#define NVME_CACHE_HASHMAP_BUCKET_COUNT 15

void storage_cache_create(struct storage_cache *const cache) {
    cache->items =
        HASHMAP_INIT(sizeof(struct storage_cache_item),
                     NVME_CACHE_HASHMAP_BUCKET_COUNT,
                     hashmap_no_hash,
                     /*hash_cb_info=*/NULL);

    cache->lock = SPINLOCK_INIT();
    cache->most_recent = NULL;
}

void
storage_cache_push(struct storage_cache *const cache,
                   const uint64_t lba,
                   void *const block)
{
    preempt_disable();
    spin_acquire(&cache->lock);

    const bool result =
        hashmap_add(&cache->items, hashmap_key_create(lba), &block);

    spin_release(&cache->lock);
    preempt_enable();

    if (!result) {
        printk(LOGLEVEL_WARN,
               "nvme: attempting to push already-cached item at "
               "lba %" PRIu64 "\n",
               lba);
    }
}

void *
storage_cache_find(struct storage_cache *const cache, const uint64_t lba) {
    preempt_disable();
    spin_acquire(&cache->lock);

    struct storage_cache_item *const most_recent = cache->most_recent;
    if (most_recent != NULL) {
        if (most_recent->lba == lba) {
            spin_release(&cache->lock);
            preempt_enable();

            return most_recent->block;
        }
    }

    void *result = NULL;
    struct storage_cache_item *const item =
        hashmap_get(&cache->items, hashmap_key_create(lba));

    if (item != NULL) {
        result = item->block;
        cache->most_recent = item;
    }

    spin_release(&cache->lock);
    preempt_enable();

    return result;
}

void storage_cache_destroy(struct storage_cache *const cache) {
    spinlock_deinit(&cache->lock);
    hashmap_destroy(&cache->items);

    cache->most_recent = NULL;
}
