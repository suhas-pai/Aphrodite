/*
 * kernel/src/mm/mm_types.h
 * © suhas pai
 */

#pragma once

#include "lib/macros.h"
#include "lib/overflow.h"

#include "mm/types.h"

#define SIZEOF_STRUCTPAGE (sizeof(uint64_t) * 5)
#define PAGE_SIZE (1ull << PAGE_SHIFT)
#define LARGEPAGE_SIZE(index) (1ull << LARGEPAGE_SHIFTS[index])
#define INVALID_PHYS (uint64_t)-1

#define PAGE_COUNT(size) ((size) >> PAGE_SHIFT)

#define SECTION_SHIFT (sizeof_bits(uint32_t) - sizeof_bits(uint8_t))
#define SECTION_MASK 0xFF

#define __page_aligned __aligned(PAGE_SIZE)

uint64_t phys_to_pfn(uint64_t phys);
uint64_t page_to_phys(const struct page *page);
uint64_t pfn_to_phys_manual(uint64_t pfn);

struct page;

#define verify_page_pointer(p) ({ \
    __auto_type __verp = (uint64_t)(p); \
    __verp >= PAGE_OFFSET && __verp < PAGE_END \
 && ((__verp - PAGE_OFFSET) % sizeof(struct page)) == 0; \
})

#define pfn_to_phys(pfn) page_to_phys(pfn_to_page(pfn))
#define pfn_to_page(pfn) ({ \
    __auto_type h_var(page) = \
        PAGE_OFFSET + check_mul_assert(SIZEOF_STRUCTPAGE, (pfn)); \
    assert_msg(verify_page_pointer(h_var(page)), \
               "pfn_to_page(): pfn %" PRIu64 " reaches outside range of page " \
               "range", \
               pfn); \
    (struct page *)h_var(page); \
})

#define page_to_pfn(p) \
    _Generic((p), \
        const struct page *: ({ \
            const uint64_t h_var(page) = (uint64_t)(p); \
            assert_msg(verify_page_pointer(h_var(page)), \
                       "page_to_pfn(): page %p is invalid", \
                       p); \
            check_sub_assert(h_var(page), PAGE_OFFSET) / SIZEOF_STRUCTPAGE; \
        }), \
        struct page *: ({ \
            const uint64_t h_var(page) = (uint64_t)(p); \
            assert_msg(verify_page_pointer(h_var(page)), \
                       "page_to_pfn(): page %p is invalid", \
                       p); \
            check_sub_assert(h_var(page), PAGE_OFFSET) / SIZEOF_STRUCTPAGE; \
        }) \
    )

#define phys_to_page(phys) pfn_to_page(phys_to_pfn(phys))
#define virt_to_page(virt) pfn_to_page(virt_to_pfn(virt))
#define virt_to_pfn(virt) phys_to_pfn(virt_to_phys(virt))
#define page_to_virt(p) \
    _Generic((p), \
        const struct page *: ({ \
            __auto_type h_var(page) = (p); \
            assert_msg(verify_page_pointer(h_var(page)), \
                       "page_to_virt(): page %p is invalid", \
                       h_var(page)); \
            phys_to_virt(page_to_phys(h_var(page))); \
        }), \
        struct page *: ({ \
            __auto_type h_var(page) = (p); \
            assert_msg(verify_page_pointer(h_var(page)), \
                       "page_to_virt(): page %p is invalid", \
                       h_var(page)); \
            phys_to_virt(page_to_phys(h_var(page))); \
        }) \
    )

typedef uint8_t pgt_level_t;
typedef uint16_t pgt_index_t;

#define PGT_LEVEL_FMT "%" PRIu8
#define PGT_INDEX_FMT "%" PRIu16

pgt_level_t pgt_get_top_level();
uint64_t sign_extend_virt_addr(uint64_t virt);

bool pte_is_present(pte_t pte);
bool pte_level_can_have_large(pgt_level_t level);
bool pte_is_large(pte_t pte);
bool pte_is_dirty(pte_t pte);

#define pte_to_pfn(pte, level) phys_to_pfn(pte_to_phys(pte, level))
#define pte_to_virt(pte, level) phys_to_virt(pte_to_phys(pte, level))
#define pte_to_page(pte, level) pfn_to_page(pte_to_pfn(pte, level))

void *phys_to_virt(uint64_t phys);
uint64_t virt_to_phys(volatile const void *phys);

extern uint64_t KERNEL_BASE;
extern uint64_t HHDM_OFFSET;

void pagezones_init();

__debug_optimize(3) static inline
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
