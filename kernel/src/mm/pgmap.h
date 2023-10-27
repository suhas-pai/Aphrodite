/*
 * kernel/mm/pgmap.h
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