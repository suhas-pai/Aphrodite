/*
 * kernel/arch/aarch64/mm/vma.c
 * © suhas pai
 */

#include "lib/align.h"
#include "mm/pgmap.h"

static inline uint64_t
flags_from_info(struct pagemap *const pagemap,
                const prot_t prot,
                const enum vma_cachekind cachekind)
{
    uint64_t result = __PTE_VALID | __PTE_INNER_SH | __PTE_ACCESS;
    if (pagemap != &kernel_pagemap) {
        result |= __PTE_NONGLOBAL | __PTE_USER;
    }

    if ((prot & PROT_WRITE) == 0) {
        result |= __PTE_RO;
    }

    if ((prot & PROT_EXEC) == 0) {
        result |= __PTE_UXN | __PTE_PXN;
    }

    switch (cachekind) {
        case VMA_CACHEKIND_WRITEBACK:
            break;
        case VMA_CACHEKIND_WRITETHROUGH:
            result |= __PTE_WT;
            break;
        case VMA_CACHEKIND_WRITECOMBINING:
            result |= __PTE_WC;
            break;
        case VMA_CACHEKIND_NO_CACHE:
            result |= __PTE_UNPREDICT;
            break;
        case VMA_CACHEKIND_MMIO:
            result |= __PTE_MMIO;
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

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3,

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