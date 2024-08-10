/*
 * kernel/src/mm/vma.c
 * © suhas pai
 */

#include "lib/align.h"

#include "kmalloc.h"
#include "pagemap.h"

__debug_optimize(3) struct vm_area *vma_prev(struct vm_area *const vma) {
    struct addrspace_node *const node = addrspace_node_prev(&vma->node);
    if (node == NULL) {
        return NULL;
    }

    return container_of(node, struct vm_area, node);
}

__debug_optimize(3) struct vm_area *vma_next(struct vm_area *const vma) {
    struct addrspace_node *const node = addrspace_node_next(&vma->node);
    if (node == NULL) {
        return NULL;
    }

    return container_of(node, struct vm_area, node);
}

struct vm_area *
vma_alloc(struct pagemap *const pagemap,
          const struct range range,
          const prot_t prot,
          const enum vma_cachekind cachekind)
{
    struct vm_area *const vma = kmalloc(sizeof(struct vm_area));
    if (vma == NULL) {
        return NULL;
    }

    vma->node = ADDRSPACE_NODE_INIT(vma->node, &pagemap->addrspace);
    vma->node.range = range;
    vma->cachekind = cachekind;
    vma->prot = prot;

    return vma;
}

struct vm_area *
vma_create(struct pagemap *const pagemap,
           const struct range in_range,
           struct range phys_range,
           const uint64_t align,
           const prot_t prot,
           const enum vma_cachekind cachekind)
{
    assert(range_has_align(phys_range, PAGE_SIZE));
    struct vm_area *const vma = vma_alloc(pagemap, phys_range, prot, cachekind);

    if (vma == NULL) {
        return NULL;
    }

    if (!pagemap_find_space_and_add_vma(pagemap,
                                        vma,
                                        in_range,
                                        phys_range.front,
                                        align))
    {
        kfree(vma);
        return NULL;
    }

    return vma;
}

struct vm_area *
vma_create_at(struct pagemap *const pagemap,
              const struct range range,
              const uint64_t phys_addr,
              const prot_t prot,
              const enum vma_cachekind cachekind)
{
    struct vm_area *const vma = vma_alloc(pagemap, range, prot, cachekind);
    if (vma == NULL) {
        return NULL;
    }

    if (!pagemap_add_vma(pagemap, vma, phys_addr)) {
        kfree(vma);
        return NULL;
    }

    return vma;
}

__debug_optimize(3) struct pagemap *vma_pagemap(struct vm_area *const vma) {
    return container_of(vma->node.addrspace, struct pagemap, addrspace);
}