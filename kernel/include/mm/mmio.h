/*
 * kernel/src/mm/mmio.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/addrspace.h"
#include "mm_types.h"

struct mmio_region {
    struct addrspace_node node;

    volatile void *base;
    uint32_t size;
};

struct range mmio_region_get_range(const struct mmio_region *region);

enum vmap_mmio_flags {
    __VMAP_MMIO_WT = 1 << 0
};

struct mmio_region *
vmap_mmio(struct range phys_range, prot_t prot, uint64_t flags);

bool vunmap_mmio(struct mmio_region *region);
