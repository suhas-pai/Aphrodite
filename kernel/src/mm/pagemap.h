/*
 * kernel/mm/pagemap.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/addrspace.h"

#include "page.h"
#include "vma.h"

struct pagemap {
#if defined(__aarch64__)
    pte_t *lower_root;
    pte_t *higher_root;
#else
    pte_t *root;
#endif /* defined(__aarch64__) */

    struct list cpu_list;
    struct spinlock cpu_lock;

    struct refcount refcount;

    struct address_space addrspace;
    struct spinlock addrspace_lock;
};

#if defined(__aarch64__)
    struct pagemap pagemap_create(pte_t *lower_root, pte_t *higher_root);
#else
    struct pagemap pagemap_create(pte_t *root);
#endif

extern struct pagemap kernel_pagemap;

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

void switch_to_pagemap(struct pagemap *pagemap);