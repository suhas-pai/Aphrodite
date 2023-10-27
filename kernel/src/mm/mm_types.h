/*
 * kernel/mm/mm_types.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/assert.h"
#include "mm/types.h"

#define SIZEOF_STRUCTPAGE (sizeof(uint64_t) * 5)
#define PAGE_SIZE (1ull << PAGE_SHIFT)
#define LARGEPAGE_SIZE(index) (1ull << LARGEPAGE_SHIFTS[index])
#define INVALID_PHYS (uint64_t)-1

#define PAGE_COUNT(size) (((uint64_t)(size) / PAGE_SIZE))

#define SECTION_SHIFT (sizeof_bits(uint32_t) - sizeof_bits(uint8_t))
#define SECTION_MASK 0xFF

#define __page_aligned __aligned(PAGE_SIZE)

uint64_t phys_to_pfn(uint64_t phys);
uint64_t page_to_phys(const struct page *page);

struct page;

#define pfn_to_phys(pfn) page_to_phys(pfn_to_page(pfn))
#define pfn_to_page(pfn) \
    ((struct page *)(PAGE_OFFSET + (SIZEOF_STRUCTPAGE * (pfn))))

#define page_to_pfn(p) \
    _Generic((p), \
        const struct page *: \
            (check_sub_assert((uint64_t)(p), PAGE_OFFSET) / SIZEOF_STRUCTPAGE),\
        struct page *: \
            (check_sub_assert((uint64_t)(p), PAGE_OFFSET) / SIZEOF_STRUCTPAGE))

#define phys_to_page(phys) pfn_to_page(phys_to_pfn(phys))
#define virt_to_page(virt) pfn_to_page(virt_to_pfn(virt))
#define virt_to_pfn(virt) phys_to_pfn(virt_to_phys(virt))
#define page_to_virt(p) \
    _Generic((p), \
        const struct page *: phys_to_virt(page_to_phys(p)), \
        struct page *: phys_to_virt(page_to_phys(p)))

typedef uint8_t pgt_level_t;
typedef uint16_t pgt_index_t;

pgt_level_t pgt_get_top_level();

bool pte_is_present(pte_t pte);
bool pte_level_can_have_large(pgt_level_t level);
bool pte_is_large(pte_t pte);
bool pte_is_dirty(pte_t pte);

#define pte_to_pfn(pte) phys_to_pfn(pte_to_phys(pte))
#define pte_to_virt(pte) phys_to_virt(pte_to_phys(pte))
#define pte_to_page(pte) pfn_to_page(pte_to_pfn(pte))

void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(volatile const void *phys);

extern uint64_t KERNEL_BASE;
extern uint64_t HHDM_OFFSET;

void pagezones_init();

static inline
uint16_t virt_to_pt_index(const uint64_t virt, const pgt_level_t level) {
    return (virt >> PAGE_SHIFTS[level - 1]) & PT_LEVEL_MASKS[level];
}

extern const uint64_t PAGE_OFFSET;
extern const uint64_t VMAP_BASE;
extern const uint64_t VMAP_END;

extern uint64_t PAGE_END;
extern uint64_t PAGING_MODE;

enum prot_flags {
    PROT_NONE,

    PROT_READ = 1 << 0,
    PROT_WRITE = 1 << 1,
    PROT_EXEC = 1 << 2,
    PROT_USER = 1 << 3,

    PROT_RW = PROT_READ | PROT_WRITE,
    PROT_WX = PROT_WRITE | PROT_EXEC,
    PROT_RX = PROT_READ | PROT_EXEC,

    PROT_RWX = PROT_READ | PROT_WRITE | PROT_EXEC,

#if defined(__aarch64__)
    PROT_DEVICE = 1 << 4
#elif defined(__riscv64)
    PROT_IO = 1 << 4
#endif
};

typedef uint8_t prot_t;

enum vma_cachekind {
    VMA_CACHEKIND_WRITEBACK,
    VMA_CACHEKIND_DEFAULT = VMA_CACHEKIND_WRITEBACK,

    VMA_CACHEKIND_WRITETHROUGH,
    VMA_CACHEKIND_WRITECOMBINING,
    VMA_CACHEKIND_NO_CACHE,

#if defined(__aarch64__) || defined(__riscv64)
    VMA_CACHEKIND_MMIO,
#else
    VMA_CACHEKIND_MMIO = VMA_CACHEKIND_NO_CACHE,
#endif /* defined(__x86_64__) */
};

pte_t pte_read(const pte_t *pte);
void pte_write(pte_t *pte, pte_t value);

bool pte_flags_equal(pte_t pte, pgt_level_t level, uint64_t flags);

void zero_page(void *page);
void zero_multiple_pages(void *page, uint64_t count);