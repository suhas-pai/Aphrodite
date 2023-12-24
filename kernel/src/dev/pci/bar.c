/*
 * kernel/src/dev/pci/bar.c
 * © suhas pai
 */

#include "dev/printk.h"

#include "sys/mmio.h"
#include "sys/pio.h"

#include "bar.h"
#include "resource.h"
#include "entity.h"

bool pci_map_bar(struct pci_entity_bar_info *const bar) {
    if (!bar->is_mmio) {
        printk(LOGLEVEL_WARN, "pcie: pci_map_bar() called on non-mmio bar\n");
        return false;
    }

    if (bar->mmio != NULL) {
        return true;
    }

    uint64_t flags = 0;
    if (bar->is_prefetchable) {
        flags |= __VMAP_MMIO_WT;
    }

    const struct range phys_range = bar->port_or_phys_range;
    struct range aligned_range = RANGE_EMPTY();

    if (!range_align_out(phys_range, PAGE_SIZE, &aligned_range)) {
        printk(LOGLEVEL_WARN,
               "pcie: failed to align mmio bar, range couldn't be aligned\n");
        return false;
    }

    struct mmio_region *const mmio =
        vmap_mmio(aligned_range, PROT_READ | PROT_WRITE, flags);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "pcie: failed to mmio map bar at phys range: " RANGE_FMT "\n",
               RANGE_FMT_ARGS(bar->port_or_phys_range));
        return false;
    }

    const uint64_t index_in_map = phys_range.front - aligned_range.front;

    bar->mmio = mmio;
    bar->index_in_mmio = index_in_map;

    return true;
}

__optimize(3) bool pci_unmap_bar(struct pci_entity_bar_info *const bar) {
    if (!bar->is_mmio) {
        printk(LOGLEVEL_WARN, "pcie: pci_unmap_bar() called on non-mmio bar\n");
        return false;
    }

    if (bar->mmio == NULL) {
        printk(LOGLEVEL_WARN, "pcie: pci_unmap_bar() called on unmapped bar\n");
        return true;
    }

    vunmap_mmio(bar->mmio);

    bar->mmio = NULL;
    bar->index_in_mmio = 0;

    return true;
}

__optimize(3) static inline volatile void *
find_ptr_in_bus_resource(struct pci_entity_info *const entity,
                         const uint32_t offset)
{
    array_foreach(&entity->bus->resources, const struct pci_bus_resource, res) {
        const struct range child_range = RANGE_INIT(res->child_base, res->size);
        if (range_has_loc(child_range, offset)) {
            return res->mmio->base + range_index_for_loc(child_range, offset);
        }
    }

    return NULL;
}

__optimize(3) uint8_t
pci_entity_bar_read8(struct pci_entity_info *const entity,
                     struct pci_entity_bar_info *const bar,
                     const uint32_t offset)
{
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to read uint8 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        return mmio_read_8(bar->mmio->base + bar->index_in_mmio + offset);
    }

#if !defined(__x86_64__)
    volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
    assert_msg(ptr != NULL,
               "pci: trying to read uint8 at offset 0x%" PRIx32 " that's not "
               "part of any resource",
               offset);

    return pio_read8((port_t)ptr);
#else
    (void)entity;
    return pio_read8(bar->port_or_phys_range.front + offset);
#endif /* !defined(__x86_64__) */
}

__optimize(3) uint16_t
pci_entity_bar_read16(struct pci_entity_info *const entity,
                      struct pci_entity_bar_info *const bar,
                      const uint32_t offset)
{
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to read uint16 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        return mmio_read_16(bar->mmio->base + bar->index_in_mmio + offset);
    }

#if !defined(__x86_64__)
    volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
    assert_msg(ptr != NULL,
               "pci: trying to read uint16 at offset 0x%" PRIx32 " that's not "
               "part of any resource",
               offset);

    return pio_read16((port_t)ptr);
#else
    (void)entity;
    return pio_read16(bar->port_or_phys_range.front + offset);
#endif /* !defined(__x86_64__) */
}

__optimize(3) uint32_t
pci_entity_bar_read32(struct pci_entity_info *const entity,
                      struct pci_entity_bar_info *const bar,
                      const uint32_t offset)
{
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to read uint32 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        return mmio_read_32(bar->mmio->base + bar->index_in_mmio + offset);
    }

#if !defined(__x86_64__)
    volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
    assert_msg(ptr != NULL,
               "pci: trying to read uint32 at offset 0x%" PRIx32 " that's not "
               "part of any resource",
               offset);

    return pio_read32((port_t)ptr);
#else
    (void)entity;
    return pio_read32(bar->port_or_phys_range.front + offset);
#endif /* !defined(__x86_64__) */
}

__optimize(3) uint64_t
pci_entity_bar_read64(struct pci_entity_info *const entity,
                      struct pci_entity_bar_info *const bar,
                      const uint32_t offset)
{
#if defined(__x86_64__)
    (void)entity;

    assert(bar->is_mmio);
    assert_msg(bar->mmio != NULL,
               "pci: trying to read uint64 at offset 0x%" PRIx32 " from "
               "bar that isn't mapped",
               offset);

    return mmio_read_64(bar->mmio->base + bar->index_in_mmio + offset);
#else
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to read uint64 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        return mmio_read_64(bar->mmio->base + bar->index_in_mmio + offset);
    }

    volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
    assert_msg(ptr != NULL,
               "pci: trying to read uint64 at offset 0x%" PRIx32 " that's not "
               "part of any resource",
               offset);

    return pio_read64((port_t)ptr);
#endif /* defined(__x86_64__) */
}

__optimize(3) void
pci_entity_bar_write8(struct pci_entity_info *const entity,
                      struct pci_entity_bar_info *const bar,
                      const uint32_t offset,
                      const uint8_t value)
{
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to write uint32 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        mmio_write_8(bar->mmio->base + bar->index_in_mmio + offset, value);
    } else {
    #if !defined(__x86_64__)
        volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
        assert_msg(ptr != NULL,
                   "pci: trying to write uint8 at offset 0x%" PRIx32 " that's "
                   "not part of any resource",
                   offset);

        pio_write8((port_t)ptr, value);
    #else
        (void)entity;
        pio_write8(bar->port_or_phys_range.front + offset, value);
    #endif /* !defined(__x86_64__) */
    }
}

__optimize(3) void
pci_entity_bar_write16(struct pci_entity_info *const entity,
                       struct pci_entity_bar_info *const bar,
                       const uint32_t offset,
                       const uint16_t value)
{
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to write uint16 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        mmio_write_16(bar->mmio->base + bar->index_in_mmio + offset, value);
    } else {
    #if !defined(__x86_64__)
        volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
        assert_msg(ptr != NULL,
                   "pci: trying to write uint16 at offset 0x%" PRIx32 " that's "
                   "not part of any resource",
                   offset);

        pio_write16((port_t)ptr, value);
    #else
        (void)entity;
        pio_write16(bar->port_or_phys_range.front + offset, value);
    #endif /* !defined(__x86_64__) */
    }
}

__optimize(3) void
pci_entity_bar_write32(struct pci_entity_info *const entity,
                       struct pci_entity_bar_info *const bar,
                       const uint32_t offset,
                       const uint32_t value)
{
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to write uint64 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        mmio_write_32(bar->mmio->base + bar->index_in_mmio + offset, value);
    } else {
    #if !defined(__x86_64__)
        volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
        assert_msg(ptr != NULL,
                   "pci: trying to write uint32 at offset 0x%" PRIx32 " that's "
                   "not part of any resource",
                   offset);

        pio_write32((port_t)ptr, value);
    #else
        (void)entity;
        pio_write32(bar->port_or_phys_range.front + offset, value);
    #endif /* !defined(__x86_64__) */
    }
}

__optimize(3) void
pci_entity_bar_write64(struct pci_entity_info *const entity,
                       struct pci_entity_bar_info *const bar,
                       const uint32_t offset,
                       const uint64_t value)
{
#if defined(__x86_64__)
    (void)entity;

    assert(bar->is_mmio);
    assert_msg(bar->mmio != NULL,
               "pci: trying to write uint64 at offset 0x%" PRIx32 " from "
               "bar that isn't mapped",
               offset);

    mmio_write_64(bar->mmio->base + bar->index_in_mmio + offset, value);
#else
    if (bar->is_mmio) {
        assert_msg(bar->mmio != NULL,
                   "pci: trying to read uint64 at offset 0x%" PRIx32 " from "
                   "bar that isn't mapped",
                   offset);

        mmio_write_64(bar->mmio->base + bar->index_in_mmio + offset, value);
    } else {
        volatile void *const ptr = find_ptr_in_bus_resource(entity, offset);
        assert_msg(ptr != NULL,
                   "pci: trying to write uint64 at offset 0x%" PRIx32 " that's "
                   "not part of any resource",
                   offset);

        pio_write64((port_t)ptr, value);
    }
#endif /* defined(__x86_64__) */
}