/*
 * kernel/src/dev/pci/ecam.c
 * © suhas pai
 */

#include "cpu/spinlock.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/mmio.h"
#include "ecam.h"

static struct list g_ecam_entity_list = LIST_INIT(g_ecam_entity_list);
static struct spinlock g_ecam_space_lock = SPINLOCK_INIT();

static uint32_t g_ecam_entity_count = 0;

__optimize(3)
static inline uint64_t map_size_for_bus_range(const struct range bus_range) {
    return bus_range.size << 20;
}

struct pci_ecam_space *
pci_add_ecam_space(const struct range bus_range,
                   const uint64_t base_addr,
                   const uint16_t segment)
{
    struct pci_ecam_space *const ecam_space = kmalloc(sizeof(*ecam_space));
    if (ecam_space == NULL) {
        return NULL;
    }

    list_init(&ecam_space->list);
    list_init(&ecam_space->space.entity_list);

    ecam_space->space.kind = PCI_SPACE_ECAM;
    ecam_space->mmio =
        vmap_mmio(RANGE_INIT(base_addr, map_size_for_bus_range(bus_range)),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (ecam_space->mmio == NULL) {
        kfree(ecam_space);
        printk(LOGLEVEL_WARN, "pci-ecam: failed to mmio-map config-space\n");

        return NULL;
    }

    ecam_space->bus_range = bus_range;
    ecam_space->space.segment = segment;

    const int flag = spin_acquire_with_irq(&g_ecam_space_lock);

    list_add(&g_ecam_entity_list, &ecam_space->list);
    g_ecam_entity_count++;

    spin_release_with_irq(&g_ecam_space_lock, flag);
    if (!pci_add_space(&ecam_space->space)) {
        return NULL;
    }

    return ecam_space;
}

__optimize(3)
bool pci_remove_ecam_space(struct pci_ecam_space *const ecam_space) {
    if (!pci_remove_space(&ecam_space->space)) {
        return false;
    }

    vunmap_mmio(ecam_space->mmio);
    const int flag = spin_acquire_with_irq(&g_ecam_space_lock);

    list_remove(&ecam_space->list);
    g_ecam_entity_count--;

    spin_release_with_irq(&g_ecam_space_lock, flag);
    return true;
}

__optimize(3) uint64_t
pci_ecam_space_loc_get_offset(const struct pci_ecam_space *const space,
                              const struct pci_space_location *const loc)
{
    return
        (range_index_for_loc(space->bus_range, loc->bus) << 20) |
        (loc->slot << 15) |
        (loc->function << 12);
}

__optimize(3) uint8_t
pci_ecam_read_8(struct pci_ecam_space *const space,
                const struct pci_space_location *const loc,
                const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint8_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    return mmio_read_8(space->mmio->base + offset);
}

__optimize(3) uint16_t
pci_ecam_read_16(struct pci_ecam_space *const space,
                 const struct pci_space_location *const loc,
                 const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint16_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    return mmio_read_16(space->mmio->base + offset);
}

__optimize(3) uint32_t
pci_ecam_read_32(struct pci_ecam_space *const space,
                 const struct pci_space_location *const loc,
                 const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint32_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    return mmio_read_32(space->mmio->base + offset);
}

__optimize(3) uint64_t
pci_ecam_read_64(struct pci_ecam_space *const space,
                 const struct pci_space_location *const loc,
                 const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint64_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    return mmio_read_64(space->mmio->base + offset);
}

__optimize(3) void
pci_ecam_write_8(struct pci_ecam_space *const space,
                 const struct pci_space_location *const loc,
                 const uint16_t off,
                 const uint8_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint8_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    mmio_write_8(space->mmio->base + offset, value);
}

__optimize(3) void
pci_ecam_write_16(struct pci_ecam_space *const space,
                  const struct pci_space_location *const loc,
                  const uint16_t off,
                  const uint16_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint16_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    mmio_write_16(space->mmio->base + offset, value);
}

__optimize(3) void
pci_ecam_write_32(struct pci_ecam_space *const space,
                  const struct pci_space_location *const loc,
                  const uint16_t off,
                  const uint32_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint32_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    mmio_write_32(space->mmio->base + offset, value);
}

__optimize(3) void
pci_ecam_write_64(struct pci_ecam_space *const space,
                  const struct pci_space_location *const loc,
                  const uint16_t off,
                  const uint64_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint64_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_space_loc_get_offset(space, loc) + off;
    mmio_write_64(space->mmio->base + offset, value);
}

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'reg' property in dtb node is missing\n");
        return false;
    }

    if (array_empty(reg_prop->list)) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'reg' property in dtb node is empty\n");
        return false;
    }

    const struct devicetree_prop_bus_range *const bus_range_prop =
        (const struct devicetree_prop_bus_range *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_PCI_BUS_RANGE);

    if (bus_range_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'bus-range' property in dtb node is missing\n");
        return false;
    }

    const struct range bus_range = bus_range_prop->range;
    uint64_t bus_range_end = 0;

    if (!range_get_end(bus_range, &bus_range_end)) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'bus-range' provided in dtb node overflows\n");
        return false;
    }

    if (bus_range_end > UINT8_MAX) {
        printk(LOGLEVEL_WARN,
               "pci-ecam: 'bus-range' provided in dtb node " RANGE_FMT " is "
               "too wide\n",
               RANGE_FMT_ARGS(bus_range));
        return false;
    }

    const struct devicetree_prop_reg_info *const mmio_reg =
        (const struct devicetree_prop_reg_info *)array_front(reg_prop->list);

    const struct range mmio_range =
        RANGE_INIT(mmio_reg->address, mmio_reg->size);

    if (!range_has_index_range(mmio_range,
                               RANGE_INIT(bus_range.front,
                                          map_size_for_bus_range(bus_range))))
    {
        printk(LOGLEVEL_INFO,
               "pci-ecam: bus-range " RANGE_FMT " of dtb node can't fit in "
               "provided mmio-range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(bus_range),
               RANGE_FMT_ARGS(mmio_range));
        return false;
    }

    pci_add_ecam_space(bus_range, mmio_range.front, /*segment=*/0);
    return true;
}

static const struct string_view compat_list[] = {
    SV_STATIC("pci-host-ecam-generic")
};

static const struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat_list,
    .compat_count = countof(compat_list),
};

__driver static const struct driver driver = {
    .name = "pci-ecam-driver",
    .dtb = &dtb_driver,
    .pci = NULL
};