/*
 * kernel/src/arch/x86_64/mm/init.c
 * © suhas pai
 */

#include "asm/msr.h"
#include "cpu/info.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/size.h"

#include "mm/early.h"
#include "mm/memmap.h"
#include "mm/pgmap.h"
#include "mm/walker.h"

#include "sys/boot.h"

static void
alloc_region(uint64_t virt_addr, uint64_t map_size, const uint64_t pte_flags) {
    enum pt_walker_result walker_result = E_PT_WALKER_OK;
    struct pt_walker pt_walker;

    ptwalker_create_for_pagemap(&pt_walker,
                                &kernel_pagemap,
                                virt_addr,
                                ptwalker_early_alloc_pgtable_cb,
                                /*free_pgtable=*/NULL);

    const bool supports_1gib_pages =
        get_cpu_capabilities()->supports_1gib_pages;

    do {
    try_1gib:
        if (map_size >= PAGE_SIZE_1GIB &&
            has_align(virt_addr, PAGE_SIZE_1GIB) &&
            supports_1gib_pages)
        {
            walker_result =
                ptwalker_fill_in_to(&pt_walker,
                                    /*level=*/3,
                                    /*should_ref=*/false,
                                    /*alloc_pgtable_cb_info=*/NULL,
                                    /*free_pgtable_cb_info=*/NULL);

            if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
            panic:
                panic("mm: failed to setup page-structs, ran out of memory\n");
            }

            pte_t *table = pt_walker.tables[2];
            pte_t *pte = &table[pt_walker.indices[2]];
            const pte_t *end = &table[PGT_PTE_COUNT];

            do {
                const uint64_t page =
                    early_alloc_large_page(PAGE_SIZE_1GIB / PAGE_SIZE);

                if (page == INVALID_PHYS) {
                    // We failed to alloc a 1gib page, so try 2mib pages next.
                    break;
                }

                *pte = phys_create_pte(page) | PTE_LARGE_FLAGS(3) | pte_flags;
                pte++;

                if (pte == end) {
                    pt_walker.indices[2] = PGT_PTE_COUNT - 1;
                    walker_result =
                        ptwalker_next_with_options(
                            &pt_walker,
                            /*level=*/3,
                            /*alloc_parents=*/true,
                            /*alloc_level=*/true,
                            /*should_ref=*/false,
                            /*alloc_pgtable_cb_info=*/NULL,
                            /*free_pgtable_cb_info=*/NULL);

                    if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
                        goto panic;
                    }

                    table = pt_walker.tables[2];
                    pte = table;
                    end = &table[PGT_PTE_COUNT];

                    break;
                }

                map_size -= PAGE_SIZE_1GIB;
                if (map_size < PAGE_SIZE_1GIB) {
                    pt_walker.indices[2] = pte - table;
                    goto try_2mib;
                }

                virt_addr += PAGE_SIZE_1GIB;
            } while (true);
        }

    try_2mib:
        if (map_size >= PAGE_SIZE_2MIB && has_align(virt_addr, PAGE_SIZE_2MIB))
        {
            walker_result =
                ptwalker_fill_in_to(&pt_walker,
                                    /*level=*/2,
                                    /*should_ref=*/false,
                                    /*alloc_pgtable_cb_info=*/NULL,
                                    /*free_pgtable_cb_info=*/NULL);

            if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
                goto panic;
            }

            pte_t *table = pt_walker.tables[1];
            pte_t *pte = &table[pt_walker.indices[1]];
            const pte_t *end = &table[PGT_PTE_COUNT];

            do {
                const uint64_t page =
                    early_alloc_large_page(PAGE_SIZE_2MIB / PAGE_SIZE);

                if (page == INVALID_PHYS) {
                    // We failed to alloc a 2mib page, so try 4kib pages next.
                    break;
                }

                *pte = phys_create_pte(page) | PTE_LARGE_FLAGS(2) | pte_flags;
                pte++;

                if (pte == end) {
                    const bool should_fill_in =
                        pt_walker.indices[2] != PGT_PTE_COUNT - 1;

                    pt_walker.indices[1] = PGT_PTE_COUNT - 1;
                    walker_result =
                        ptwalker_next_with_options(
                            &pt_walker,
                            /*level=*/2,
                            /*alloc_parents=*/should_fill_in,
                            /*alloc_level=*/should_fill_in,
                            /*should_ref=*/false,
                            /*alloc_pgtable_cb_info=*/NULL,
                            /*free_pgtable_cb_info=*/NULL);

                    if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
                        goto panic;
                    }

                    if (!should_fill_in) {
                        goto try_1gib;
                    }

                    table = pt_walker.tables[1];
                    pte = table;
                    end = &pte[PGT_PTE_COUNT];
                }

                map_size -= PAGE_SIZE_2MIB;
                if (map_size < PAGE_SIZE_2MIB) {
                    pt_walker.indices[1] = pte - table;
                    goto try_normal;
                }

                virt_addr += PAGE_SIZE_2MIB;
            } while (true);
        }

    try_normal:
        if (map_size != 0) {
            walker_result =
                ptwalker_fill_in_to(&pt_walker,
                                    /*level=*/1,
                                    /*should_ref=*/false,
                                    /*alloc_pgtable_cb_info=*/NULL,
                                    /*free_pgtable_cb_info=*/NULL);

            if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
                goto panic;
            }

            pte_t *table = pt_walker.tables[0];
            pte_t *pte = &table[pt_walker.indices[0]];
            const pte_t *end = &table[PGT_PTE_COUNT];

            do {
                const uint64_t page = early_alloc_page();
                if (__builtin_expect(page == INVALID_PHYS, 0)) {
                    panic("mm: failed to allocate page while setting up "
                          "kernel-pagemap\n");
                }

                *pte = phys_create_pte(page) | PTE_LEAF_FLAGS | pte_flags;
                map_size -= PAGE_SIZE;

                if (map_size == 0) {
                    return;
                }

                pte++;
                if (pte == end) {
                    const bool should_fill_in =
                        pt_walker.indices[1] != PGT_PTE_COUNT - 1;

                    pt_walker.indices[0] = PGT_PTE_COUNT - 1;
                    walker_result =
                        ptwalker_next_with_options(
                            &pt_walker,
                            /*level=*/1,
                            /*alloc_parents=*/should_fill_in,
                            /*alloc_level=*/should_fill_in,
                            /*should_ref=*/false,
                            /*alloc_pgtable_cb_info=*/NULL,
                            /*free_pgtable_cb_info=*/NULL);

                    if (__builtin_expect(walker_result != E_PT_WALKER_OK, 0)) {
                        goto panic;
                    }

                    if (!should_fill_in) {
                        if (pt_walker.indices[2] == 0) {
                            goto try_1gib;
                        }

                        goto try_2mib;
                    }

                    table = pt_walker.tables[0];
                    pte = table;
                    end = &table[PGT_PTE_COUNT];
                }

                virt_addr += PAGE_SIZE;
            } while (true);
        }
    } while (false);
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

    const uint64_t pte_flags = __PTE_WRITE | __PTE_GLOBAL | __PTE_NOEXEC;
    alloc_region(PAGE_OFFSET, map_size, pte_flags);

    PAGE_END = PAGE_OFFSET + table_size;
    printk(LOGLEVEL_INFO, "mm: finished mapping structpage-table\n");
}

static void
map_into_kernel_pagemap(const struct range phys_range,
                        const uint64_t virt_addr,
                        const uint64_t pte_flags)
{
    const bool supports_1gib = get_cpu_capabilities()->supports_1gib_pages;
    const struct pgmap_options options = {
        .pte_flags = __PTE_GLOBAL | pte_flags,

        .alloc_pgtable_cb_info = NULL,
        .free_pgtable_cb_info = NULL,

        .supports_largepage_at_level_mask = 1 << 2 | supports_1gib << 3,

        .free_pages = true,
        .is_in_early = true,
        .is_overwrite = false
    };

    assert_msg(pgmap_at(&kernel_pagemap, phys_range, virt_addr, &options),
               "mm: failed to setup kernel-pagemap");

    printk(LOGLEVEL_INFO,
           "mm: mapped " RANGE_FMT " to " RANGE_FMT "\n",
           RANGE_FMT_ARGS(phys_range),
           RANGE_FMT_ARGS(RANGE_INIT(virt_addr, phys_range.size)));
}

static void setup_kernel_pagemap(uint64_t *const kernel_memmap_size_out) {
    const uint64_t root_phys = early_alloc_page();
    if (root_phys == INVALID_PHYS) {
        panic("mm: failed to allocate root page for kernel-pagemap");
    }

    kernel_pagemap.root = phys_to_virt(root_phys);
    uint64_t kernel_memmap_size = 0;

    // Map all 'good' regions into the hhdm, except the kernel, which goes in
    // its own range.

    const struct mm_memmap *const memmap_begin = mm_get_memmap_list();
    const struct mm_memmap *const memmap_end =
        &memmap_begin[mm_get_memmap_count()];

    for (const struct mm_memmap *memmap = memmap_begin;
         memmap != memmap_end;
         memmap++)
    {
        // ACPI's RSDP pointer and other ACPI information is currently stored in
        // a reserved memmap, so map reserved memmaps in the HHDM for now.

        if (memmap->kind == MM_MEMMAP_KIND_BAD_MEMORY) {
            continue;
        }

        struct range range = memmap->range;
        if (memmap->kind == MM_MEMMAP_KIND_KERNEL_AND_MODULES) {
            kernel_memmap_size = range.size;
            map_into_kernel_pagemap(/*phys_range=*/range,
                                    /*virt_addr=*/KERNEL_BASE,
                                    __PTE_WRITE);
        } else {
            uint64_t flags = __PTE_WRITE | __PTE_NOEXEC;
            if (memmap->kind == MM_MEMMAP_KIND_FRAMEBUFFER) {
                flags |= __PTE_WC;
            }

            map_into_kernel_pagemap(/*phys_range=*/range,
                                    (uint64_t)phys_to_virt(range.front),
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
}

static void fill_kernel_pagemap_struct(const uint64_t kernel_memmap_size) {
    // Setup vma_tree to include all ranges we've mapped.
    struct vm_area *const null_area =
        vma_alloc(&kernel_pagemap,
                  range_create_upto(PAGE_SIZE),
                  PROT_NONE,
                  VMA_CACHEKIND_NO_CACHE);

    // This range was never mapped, but is still reserved.
    struct vm_area *const mmio =
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
        addrspace_add_node(&kernel_pagemap.addrspace, &mmio->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &kernel->node) &&
        addrspace_add_node(&kernel_pagemap.addrspace, &hhdm->node),
        "mm: failed to setup kernel-pagemap");
}

void mm_arch_init() {
    const uint64_t pat_msr_orig = read_msr(IA32_MSR_PAT);
    const uint64_t pat_msr =
        pat_msr_orig |
        (~((MSR_PAT_ENTRY_MASK << MSR_PAT_INDEX_PAT2) |
           (MSR_PAT_ENTRY_MASK << MSR_PAT_INDEX_PAT3)));

    printk(LOGLEVEL_INFO,
           "mm: pat msr original value is 0x%" PRIx64 "\n",
           pat_msr_orig);

    const uint64_t uncacheable_mask =
        (uint64_t)MSR_PAT_ENCODING_UNCACHEABLE << MSR_PAT_INDEX_PAT2;
    const uint64_t write_combining_mask =
        (uint64_t)MSR_PAT_ENCODING_WRITE_COMBINING << MSR_PAT_INDEX_PAT3;

    write_msr(IA32_MSR_PAT, pat_msr | uncacheable_mask | write_combining_mask);

    uint64_t kernel_memmap_size = 0;
    setup_kernel_pagemap(&kernel_memmap_size);

    mm_post_arch_init();
    fill_kernel_pagemap_struct(kernel_memmap_size);

    printk(LOGLEVEL_INFO, "mm: finished setting up\n");
}