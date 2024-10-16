/*
 * kernel/src/mm/early.h
 * © suhas pai
 */

#pragma once

#include "mm_types.h"
#include "section.h"

void mm_early_init();
void mm_post_arch_init();

void mm_init();
void mm_early_refcount_alloced_map(uint64_t virt_addr, uint64_t length);

void
mm_early_identity_map_phys(uint64_t root_phys,
                           uint64_t phys,
                           uint64_t pte_flags);

void mm_remove_early_identity_map();
uint64_t mm_get_total_page_count();

uint64_t early_alloc_page();
uint64_t early_alloc_large_page(pgt_level_t level);

void
early_free_pages_from_section(struct page *page,
                              struct page_section *section,
                              uint8_t order);
