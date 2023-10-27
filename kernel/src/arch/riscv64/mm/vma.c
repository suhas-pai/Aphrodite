/*
 * kernel/arch/riscv64/mm/vma.c
 * © suhas pai
 */

#include "mm/pgmap.h"

static inline uint64_t
flags_from_info(struct pagemap *const pagemap,
                const prot_t prot,
                const enum vma_cachekind cachekind)
{
    uint64_t sanitized_prot = prot & PROT_RWX;
    assert_msg(sanitized_prot != 0,
               "mm: arch_make_mapping(): got protections w/o any of rwx");

    uint64_t result =
        __PTE_VALID | __PTE_ACCESSED | __PTE_DIRTY | (sanitized_prot << 1);

    if (pagemap == &kernel_pagemap) {
        result |= __PTE_GLOBAL;
    } else {
        result |= __PTE_USER;
    }

    switch (cachekind) {
        case VMA_CACHEKIND_WRITEBACK:
            break;
        case VMA_CACHEKIND_WRITETHROUGH:
        case VMA_CACHEKIND_WRITECOMBINING:
        case VMA_CACHEKIND_NO_CACHE:
            break;
        case VMA_CACHEKIND_MMIO:
            break;
    }

    return result;
}

bool
arch_make_mapping(struct pagemap *const pagemap,
                  const struct range phys_range,
                  const uint64_t virt_addr,
                  const prot_t prot,
                  const enum vma_cachekind cachekind,
                  const bool is_overwrite)
{
    const struct pgmap_options options = {
        .pte_flags = flags_from_info(pagemap, prot, cachekind),

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask =
            PAGING_MODE > 3 ? (1 << 2 | 1 << 3 | 1 << 4) : (1 << 2 | 1 << 3),

        .free_pages = true,
        .is_in_early = false,
        .is_overwrite = is_overwrite
    };

    pgmap_at(pagemap, phys_range, virt_addr, &options);
    return true;
}

bool
arch_unmap_mapping(struct pagemap *const pagemap,
                   const struct range virt_range,
                   const struct pgmap_options *const map_options,
                   const struct pgunmap_options *const options)
{
    return pgunmap_at(pagemap, virt_range, map_options, options);
}