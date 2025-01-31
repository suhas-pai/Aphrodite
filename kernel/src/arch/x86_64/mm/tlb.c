/*
 * kernel/src/arch/x86_64/mm/tlb.c
 * © suhas pai
 */

#include "asm/tlb.h"

#include "mm/page_alloc.h"
#include "mm/tlb.h"

__debug_optimize(3) static void tlb_flush_range(const struct range range) {
    range_iterate(range, addr, PAGE_SIZE) {
        invlpg(addr);
    }
}

__debug_optimize(3) void tlb_flush_pageop(struct pageop *const op) {
    tlb_flush_range(op->flush_range);

    struct page *page = nullptr;
    struct page *tmp = nullptr;

    list_foreach_mut(&op->delayed_free, table.delayed_free_list, page, tmp) {
        list_deinit(&page->table.delayed_free_list);
        free_page(page);
    }
}