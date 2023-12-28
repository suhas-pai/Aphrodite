/*
 * kernel/src/mm/pgmap.h
 * © suhas pai
 */

#pragma once
#include "pagemap.h"

struct pgmap_options {
    uint64_t pte_flags;

    void *alloc_pgtable_cb_info;
    void *free_pgtable_cb_info;

    uint8_t supports_largepage_at_level_mask;

    bool free_pages : 1;
    bool is_in_early : 1;
    bool is_overwrite : 1;
};

bool
pgmap_at(struct pagemap *pagemap,
         struct range phys_range,
         uint64_t virt_addr,
         const struct pgmap_options *options);

typedef uint64_t (*pgmap_alloc_page_t)(void *cb_info);
typedef uint64_t (*pgmap_alloc_large_page_t)(pgt_level_t level, void *cb_info);

struct pgmap_alloc_options {
    pgmap_alloc_page_t alloc_page;
    pgmap_alloc_large_page_t alloc_large_page;

    void *alloc_page_cb_info;
    void *alloc_large_page_cb_info;
};

enum pgmap_alloc_result {
    E_PGMAP_ALLOC_OK,
    E_PGMAP_ALLOC_PAGE_ALLOC_FAIL,
    E_PGMAP_ALLOC_PGTABLE_ALLOC_FAIL
};

enum pgmap_alloc_result
pgmap_alloc_at(struct pagemap *pagemap,
               struct range virt_range,
               const struct pgmap_options *options,
               const struct pgmap_alloc_options *alloc_options);

struct pgunmap_options {
    bool free_pages : 1;
    bool dont_split_large_pages : 1;
};

bool
pgunmap_at(struct pagemap *pagemap,
           struct range virt_range,
           const struct pgmap_options *map_options,
           const struct pgunmap_options *unmap_options);

bool
arch_make_mapping(struct pagemap *pagemap,
                  struct range phys_range,
                  uint64_t virt_addr,
                  prot_t prot,
                  enum vma_cachekind cachekind,
                  bool is_overwrite);

bool
arch_unmap_mapping(struct pagemap *pagemap,
                   struct range virt_range,
                   const struct pgmap_options *map_options,
                   const struct pgunmap_options *options);
