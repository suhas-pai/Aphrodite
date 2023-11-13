/*
 * kernel/src/arch/aarch64/mm/init.c
 * © suhas pai
 */

#include "asm/mair.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/size.h"

#include "mm/early.h"
#include "mm/memmap.h"
#include "mm/pgmap.h"
#include "mm/walker.h"

#include "sys/boot.h"

__optimize(3) static void
alloc_region(uint64_t virt_addr, uint64_t map_size, const uint64_t pte_flags) {
    const struct pgmap_options options = {
        .pte_flags = pte_flags,
        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3 | 1 << 4,

        .free_pages = false,
        .is_in_early = true,
        .is_overwrite = false
    };

    const struct pgmap_alloc_options alloc_options = {
        .alloc_page = early_alloc_page,
        .alloc_large_page = early_alloc_large_page,

        .alloc_page_cb_info = NULL,
        .alloc_large_page_cb_info = NULL,
    };

    const enum pgmap_alloc_result map_result =
        pgmap_alloc_at(&kernel_pagemap,
                       RANGE_INIT(virt_addr, map_size),
                       &options,
                       &alloc_options);

    switch (map_result) {
        case E_PGALLOC_MAP_OK:
            break;
        case E_PGALLOC_MAP_PAGE_ALLOC_FAIL:
        case E_PGALLOC_MAP_PGTABLE_ALLOC_FAIL:
            panic("mm: ran out of free pages");
    }
}

extern uint64_t structpage_page_count;

static void setup_pagestructs_table() {
    const uint64_t table_size = structpage_page_count * SIZEOF_STRUCTPAGE;
    uint64_t map_size = table_size;

    if (!align_up(map_size, PAGE_SIZE, &map_size)) {
        panic("mm: failed to initialize memory, overflow error when aligning "
              "structpage-table size to PAGE_SIZE");
    }

    printk(LOGLEVEL_INFO,
           "mm: structpage table is %" PRIu64 " bytes\n",
           map_size);

    const uint64_t pte_flags = __PTE_PXN | __PTE_UXN;
    alloc_region(PAGE_OFFSET, map_size, pte_flags);

    PAGE_END = PAGE_OFFSET + table_size;
    printk(LOGLEVEL_INFO, "mm: finished mapping structpage-table\n");
}

static void
map_into_kernel_pagemap(struct range phys_range,
                        uint64_t virt_addr,
                        const uint64_t pte_flags)
{
    const struct pgmap_options options = {
        .pte_flags = __PTE_INNER_SH | pte_flags,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | 1 << 3,

        .free_pages = false,
        .is_in_early = true,
        .is_overwrite = false,
    };

#if defined(AARCH64_USE_16K_PAGES)
    struct range new_phys_range = RANGE_EMPTY();
    if (!range_align_in(phys_range, PAGE_SIZE, &new_phys_range)) {
        printk(LOGLEVEL_WARN,
               "mm: failed to align memmap at phys-range " RANGE_FMT ", "
               "couldn't align to 16kib page size\n",
               RANGE_FMT_ARGS(phys_range));
        return;
    }

    virt_addr += new_phys_range.front - phys_range.front;
    phys_range = new_phys_range;
#endif /* defined(AARCH64_USE_16K_PAGES) */

    assert_msg(pgmap_at(&kernel_pagemap, phys_range, virt_addr, &options),
               "mm: failed to setup kernel-pagemap");

    printk(LOGLEVEL_INFO,
           "mm: mapped " RANGE_FMT " to " RANGE_FMT "\n",
           RANGE_FMT_ARGS(phys_range),
           RANGE_FMT_ARGS(RANGE_INIT(virt_addr, phys_range.size)));
}

static void setup_kernel_pagemap(uint64_t *const kernel_memmap_size_out) {
    const uint64_t lower_root = early_alloc_page();
    if (lower_root == INVALID_PHYS) {
        panic("mm: failed to allocate lower-half root page for the "
              "kernel-pagemap");
    }

    const uint64_t higher_root = early_alloc_page();
    if (higher_root == INVALID_PHYS) {
        panic("mm: failed to allocate higher-half root page for the "
              "kernel-pagemap");
    }

    kernel_pagemap.lower_root = phys_to_virt(lower_root);
    kernel_pagemap.higher_root = phys_to_virt(higher_root);

    const struct range ident_map_range = range_create_end(kib(16), gib(4));
    map_into_kernel_pagemap(/*phys_range=*/ident_map_range,
                            /*virt_addr=*/ident_map_range.front,
                            __PTE_MMIO | __PTE_PXN | __PTE_UXN);

    // Map all 'good' regions into the hhdm
    uint64_t kernel_memmap_size = 0;
    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = &mm_get_memmap_list()[i];
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY ||
            memmap->kind == MM_MEMMAP_KIND_RESERVED)
        {
            continue;
        }

        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            kernel_memmap_size = memmap->range.size;
            map_into_kernel_pagemap(/*phys_range=*/memmap->range,
                                    /*virt_addr=*/KERNEL_BASE,
                                    /*pte_flags=*/0);
        } else {
            uint64_t flags = __PTE_PXN | __PTE_UXN;
            if (memmap->kind == MM_MEMMAP_KIND_FRAMEBUFFER) {
                flags |= __PTE_WC;
            }

            map_into_kernel_pagemap(/*phys_range=*/memmap->range,
                                    (uint64_t)phys_to_virt(memmap->range.front),
                                    flags);
        }
    }

    *kernel_memmap_size_out = kernel_memmap_size;

    setup_pagestructs_table();
    switch_to_pagemap(&kernel_pagemap);

    // All 'good' memmaps use pages with associated struct pages to them,
    // despite the pages being allocated before the structpage-table, so we have
    // to go through each table used, and setup all pgtable metadata inside a
    // struct page

    mm_early_refcount_alloced_map(ident_map_range.front, ident_map_range.size);
    mm_early_refcount_alloced_map(VMAP_BASE, (VMAP_END - VMAP_BASE));

    for (uint64_t i = 0; i != mm_get_memmap_count(); i++) {
        const struct mm_memmap *const memmap = &mm_get_memmap_list()[i];
        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY ||
            memmap->kind == MM_MEMMAP_KIND_RESERVED)
        {
            continue;
        }

        uint64_t virt_addr = 0;
        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            virt_addr = KERNEL_BASE;
        } else {
            virt_addr = (uint64_t)phys_to_virt(memmap->range.front);
        }

        mm_early_refcount_alloced_map(virt_addr, memmap->range.size);
    }

    printk(LOGLEVEL_INFO, "mm: finished setting up kernel-pagemap!\n");
}

static void fill_kernel_pagemap_struct(const uint64_t kernel_memmap_size) {
    // Setup vma_tree to include all ranges we've mapped.
    struct vm_area *const null_area =
        vma_alloc(&kernel_pagemap,
                  RANGE_INIT(0, PAGE_SIZE),
                  PROT_NONE,
                  VMA_CACHEKIND_NO_CACHE);

    // This range was never mapped, but is still reserved.
    struct vm_area *const vmap =
        vma_alloc(&kernel_pagemap,
                  RANGE_INIT(VMAP_BASE, (VMAP_END - VMAP_BASE)),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_MMIO);

    struct vm_area *const kernel =
        vma_alloc(&kernel_pagemap,
                  RANGE_INIT(KERNEL_BASE, kernel_memmap_size),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_DEFAULT);

    struct vm_area *const hhdm =
        vma_alloc(&kernel_pagemap,
                  RANGE_INIT(HHDM_OFFSET, tib(64)),
                  PROT_READ | PROT_WRITE,
                  VMA_CACHEKIND_DEFAULT);

    assert_msg(
        addrspace_add_node(&kernel_pagemap.addrspace, &null_area->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &vmap->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &kernel->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &hhdm->node),
        "mm: failed to setup kernel-pagemap");
}

static void setup_mair() {
    // Device nGnRnE
    const uint64_t device_uncacheable_encoding = 0b00000000;
    // Device GRE
    const uint64_t device_write_combining_encoding = 0b00001100;
    // Normal memory, inner and outer non-cacheable
    const uint64_t memory_uncacheable_encoding = 0b01000100;
    // Normal memory inner and outer writethrough, non-transient
    const uint64_t memory_writethrough_encoding = 0b10111011;
    // Normal memory, inner and outer write-back, non-transient
    const uint64_t memory_write_back_encoding = 0b11111111;

    const uint64_t mair_value =
        memory_write_back_encoding |
        device_write_combining_encoding << 8 |
        memory_writethrough_encoding << 16 |
        device_uncacheable_encoding << 24 |
        memory_uncacheable_encoding << 32;

    write_mair_el1(mair_value);
}

void mm_arch_init() {
    setup_mair();

    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(&kernel_memmap_size);

    mm_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}