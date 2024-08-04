/*
 * kernel/src/mm/pagemap.h
 * © suhas pai
 */

#pragma once

#include "lib/refcount.h"
#include "vma.h"

#if defined(__aarch64__) || defined(__loongarch64)
    #define PAGEMAP_HAS_SPLIT_ROOT 1
#else
    #define PAGEMAP_HAS_SPLIT_ROOT 0
#endif

struct pagemap {
#if defined(__aarch64__) || defined(__loongarch64)
    pte_t *lower_root;
    pte_t *higher_root;
#else
    pte_t *root;
#endif /* defined(__aarch64__) */

    struct address_space addrspace;
    struct spinlock addrspace_lock;

    struct list cpu_list;
    struct spinlock cpu_lock;

    struct refcount refcount;
};

struct pagemap pagemap_empty();

#if PAGEMAP_HAS_SPLIT_ROOT
    struct pagemap pagemap_create(pte_t *lower_root, pte_t *higher_root);
#else
    struct pagemap pagemap_create(pte_t *root);
#endif /* PAGEMAP_HAS_SPLIT_ROOT */

bool
pagemap_find_space_and_add_vma(struct pagemap *pagemap,
                               struct vm_area *vma,
                               struct range in_range,
                               uint64_t phys_addr,
                               uint64_t align);

bool
pagemap_add_vma(struct pagemap *pagemap,
                struct vm_area *vma,
                uint64_t phys_addr);

uint64_t pagemap_virt_get_phys(const struct pagemap *pagemap, uint64_t virt);
void switch_to_pagemap(struct pagemap *pagemap);
