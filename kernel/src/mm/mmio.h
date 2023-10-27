/*
 * kernel/mm/mmio.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/addrspace.h"
#include "mm/mm_types.h"

struct mmio_region {
    struct addrspace_node node;

    volatile void *base;
    uint32_t size;

    // Internal flags
    uint32_t flags;
};

struct range mmio_region_get_range(const struct mmio_region *region);

enum vmap_mmio_flags {
    __VMAP_MMIO_WT = 1 << 0
};

struct mmio_region *
vmap_mmio_low4g(prot_t prot, uint8_t order, uint64_t flags);

struct mmio_region *
vmap_mmio(struct range phys_range, prot_t prot, uint64_t flags);

bool vunmap_mmio(struct mmio_region *region);
