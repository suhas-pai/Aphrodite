/*
 * kernel/src/arch/loongarch64/mm/vma.c
 * © suhas pai
 */

#include "asm/mat.h"
#include "asm/privl.h"

#include "cpu/info.h"

#include "mm/pgmap.h"
#include "sched/process.h"

static inline uint64_t
flags_from_info(struct pagemap *const pagemap,
                const prot_t prot,
                const enum vma_cachekind cachekind,
                uint64_t *const large_flags)
{
    uint64_t result = __PTE_VALID;
    if (pagemap == &kernel_process.pagemap) {
        result |= __PTE_GLOBAL;
    } else {
        result |= PRIVL_USER << PTE_PRIVL_SHIFT;
    }

    if (prot & PROT_WRITE) {
        result |= __PTE_WRITE;
    }

    if ((prot & PROT_EXEC) == 0) {
        result |= __PTE_NO_EXEC;
    }

    if ((prot & PROT_READ) == 0) {
        result |= __PTE_NO_READ;
    }

    switch (cachekind) {
        case VMA_CACHEKIND_WRITEBACK:
            break;
        case VMA_CACHEKIND_WRITETHROUGH:
            result |=
                MEM_ACCESS_CTRL_CACHE_COHERENT << PTE_MEM_ACCESS_CTRL_SHIFT;
            break;
        case VMA_CACHEKIND_WRITECOMBINING:
            result |=
                MEM_ACCESS_CTRL_WEAKLY_CACHE_COHERENT
                    << PTE_MEM_ACCESS_CTRL_SHIFT;
            break;
        case VMA_CACHEKIND_NO_CACHE:
            break;
    }

    *large_flags = result;
    if (pagemap == &kernel_process.pagemap) {
        // __PTE_GLOBAL and __PTE_LARGE are the same bit, so we just need to
        // add __PTE_GLOBAL_LARGE

        *large_flags |= __PTE_GLOBAL_LARGE;
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
    struct pgmap_options options = {
        .leaf_pte_flags = 0,
        .large_pte_flags = 0,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3,

        .is_in_early = false,
        .free_pages = false,
        .is_overwrite = is_overwrite
    };

    options.leaf_pte_flags =
        flags_from_info(pagemap, prot, cachekind, &options.large_pte_flags);

    return pgmap_at(pagemap, phys_range, virt_addr, &options);
}

bool
arch_unmap_mapping(struct pagemap *const pagemap,
                   const struct range virt_range,
                   const struct pgmap_options *const map_options,
                   const struct pgunmap_options *const options)
{
    return pgunmap_at(pagemap, virt_range, map_options, options);
}
