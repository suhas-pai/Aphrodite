/*
 * kernel/src/dev/storage/nvme/cache.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/hashmap.h"
#include "cpu/spinlock.h"

enum nvme_cache_item_state {
    NVME_CACHE_ITEM_STATE_OK,
    NVME_CACHE_ITEM_STATE_DIRTY,
};

struct nvme_cache_item {
    void *block;
    uint64_t lba;
};

struct nvme_cache {
    struct hashmap items;
    struct spinlock lock;

    struct nvme_cache_item *most_recent;
};

void nvme_cache_create(struct nvme_cache *cache);
void nvme_cache_push(struct nvme_cache *cache, uint64_t lba, void *block);

void *nvme_cache_find(struct nvme_cache *cache, uint64_t lba);
void nvme_cache_destroy(struct nvme_cache *cache);
