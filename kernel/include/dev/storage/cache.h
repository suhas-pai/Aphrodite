/*
 * kernel/src/dev/storage/cache.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/hashmap.h"
#include "cpu/spinlock.h"

struct storage_cache_item {
    void *block;
    uint64_t lba;
};

struct storage_cache {
    struct hashmap items;
    struct spinlock lock;

    struct storage_cache_item *most_recent;
};

void storage_cache_create(struct storage_cache *cache);
void storage_cache_push(struct storage_cache *cache, uint64_t lba, void *block);

void *storage_cache_find(struct storage_cache *cache, uint64_t lba);
void storage_cache_destroy(struct storage_cache *cache);
