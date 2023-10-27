/*
 * kernel/mm/mmio.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/size.h"

#include "mm/pgmap.h"
#include "mm/zone.h"

#include "kmalloc.h"
#include "mmio.h"

static struct address_space mmio_space = ADDRSPACE_INIT(mmio_space);
static struct spinlock mmio_space_lock = SPINLOCK_INIT();

enum mmio_region_flags {
    __MMIO_REGION_LOW4G = 1 << 0
};

enum prot_fail {
    PROT_FAIL_NONE,
    PROT_FAIL_PROT_NONE,
    PROT_FAIL_PROT_EXEC,
    PROT_FAIL_PROT_USER
};

__optimize(3) static inline enum prot_fail verify_prot(const prot_t prot) {
    if (prot == PROT_NONE) {
        return PROT_FAIL_PROT_NONE;
    }

    if (prot & PROT_EXEC) {
        return PROT_FAIL_PROT_EXEC;
    }

    if (prot & PROT_USER) {
        return PROT_FAIL_PROT_USER;
    }

    return PROT_FAIL_NONE;
}

// 16kib of guard pages
#define GUARD_PAGE_SIZE min(kib(16), PAGE_SIZE)

struct mmio_region *
map_mmio_region(const struct range phys_range,
                const prot_t prot,
                const uint64_t flags)
{
    struct range in_range =
        range_create_end(VMAP_BASE + GUARD_PAGE_SIZE, VMAP_END);

    struct mmio_region *const mmio = kmalloc(sizeof(*mmio));
    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to allocate mmio_region to map phys-range "
               RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    mmio->node = ADDRSPACE_NODE_INIT(mmio->node, &mmio_space);
    mmio->node.range.size = phys_range.size + GUARD_PAGE_SIZE;

    const int flag = spin_acquire_with_irq(&mmio_space_lock);
    const uint64_t virt_addr =
        addrspace_find_space_and_add_node(&mmio_space,
                                          in_range,
                                          &mmio->node,
                                          /*align=*/PAGE_SIZE);

    if (virt_addr == ADDRSPACE_INVALID_ADDR) {
        spin_release_with_irq(&mmio_space_lock, flag);
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to find a suitable virtual-address range "
               "to map phys-range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range));

        return NULL;
    }

    const struct range virt_range = RANGE_INIT(virt_addr, phys_range.size);
    const bool map_success =
        arch_make_mapping(&kernel_pagemap,
                          phys_range,
                          virt_addr,
                          prot,
                          (flags & __VMAP_MMIO_WT) ?
                            VMA_CACHEKIND_WRITETHROUGH : VMA_CACHEKIND_MMIO,
                          /*is_overwrite=*/false);

    spin_release_with_irq(&mmio_space_lock, flag);
    if (!map_success) {
        kfree(mmio);
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): failed to map phys-range " RANGE_FMT " to virtual "
               "range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(phys_range),
               RANGE_FMT_ARGS(virt_range));

        return NULL;
    }

    mmio->base = (volatile void *)virt_addr;
    mmio->size = phys_range.size;

    return mmio;
}

struct mmio_region *
vmap_mmio_low4g(const prot_t prot, const uint8_t order, const uint64_t flags) {
    switch (verify_prot(prot)) {
        case PROT_FAIL_NONE:
            break;
        case PROT_FAIL_PROT_NONE:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio_low4g(): attempting to map a low-4g mmio range "
                   "w/o access permissions\n");
            return NULL;
        case PROT_FAIL_PROT_EXEC:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio_low4g(): attempting to map a low-4g mmio range "
                   "with execute permissions\n");
            return NULL;
        case PROT_FAIL_PROT_USER:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio_low4g(): attempting to map a low-4g mmio range "
                   "with user permissions\n");
            return NULL;
    }

    struct page *const page =
        alloc_pages_from_zone(page_zone_low4g(),
                              PAGE_STATE_USED,
                              __ALLOC_ZERO,
                              order,
                              /*fallback=*/true);

    if (page == NULL) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio_low4g(): failed to allocate low-4g pages to map a "
               "mmio range\n");
        return NULL;
    }

    const struct range phys_range =
        RANGE_INIT(page_to_phys(page), PAGE_SIZE * order);

    struct mmio_region *const mmio = map_mmio_region(phys_range, prot, flags);
    if (mmio == NULL) {
        free_pages(page, order);
        return NULL;
    }

    return mmio;
}

struct mmio_region *
vmap_mmio(const struct range phys_range,
          const prot_t prot,
          const uint64_t flags)
{
    if (range_empty(phys_range)) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): attempting to map empty phys-range\n");
        return NULL;
    }

    if (!range_has_align(phys_range, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "vmap_mmio(): phys-range " RANGE_FMT " isn't aligned to the "
               "page size\n",
               RANGE_FMT_ARGS(phys_range));
        return NULL;
    }

    switch (verify_prot(prot)) {
        case PROT_FAIL_NONE:
            break;
        case PROT_FAIL_PROT_NONE:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT " "
                   "w/o access permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
        case PROT_FAIL_PROT_EXEC:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT " "
                   "with execute permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
        case PROT_FAIL_PROT_USER:
            printk(LOGLEVEL_WARN,
                   "vmap_mmio(): attempting to map mmio range " RANGE_FMT " "
                   "with user permissions\n",
                   RANGE_FMT_ARGS(phys_range));
            return NULL;
    }

    return map_mmio_region(phys_range, prot, flags);
}

bool vunmap_mmio(struct mmio_region *const region) {
    const struct pgunmap_options options = {
        .free_pages = region->flags & __MMIO_REGION_LOW4G,
        .dont_split_large_pages = true
    };

    const struct range virt_range = mmio_region_get_range(region);

    const int flag = spin_acquire_with_irq(&mmio_space_lock);
    const bool result =
        pgunmap_at(&kernel_pagemap,
                   virt_range,
                   /*map_options=*/NULL,
                   &options);

    if (!result) {
        spin_release_with_irq(&mmio_space_lock, flag);
        printk(LOGLEVEL_WARN,
               "mm: failed to map mmio region at " RANGE_FMT "\n",
               RANGE_FMT_ARGS(virt_range));

        return false;
    }

    addrspace_remove_node(&region->node);
    spin_release_with_irq(&mmio_space_lock, flag);
    kfree(region);

    return true;
}

struct range mmio_region_get_range(const struct mmio_region *const region) {
    return RANGE_INIT((uint64_t)region->base, region->size);
}