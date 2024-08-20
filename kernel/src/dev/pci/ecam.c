/*
 * kernel/src/dev/pci/ecam.c
 * © suhas pai
 */

#include "cpu/spinlock.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/util.h"

#include "mm/kmalloc.h"
#include "sys/mmio.h"

#include "ecam.h"
#include "resource.h"

static struct list g_ecam_entity_list = LIST_INIT(g_ecam_entity_list);
static struct spinlock g_ecam_domain_lock = SPINLOCK_INIT();

static uint32_t g_ecam_entity_count = 0;

__debug_optimize(3)
static inline uint64_t map_size_for_bus_range(const struct range bus_range) {
    return bus_range.size << 20;
}

struct pci_domain_ecam *
pci_add_ecam_domain(const struct range bus_range,
                    const uint64_t base_addr,
                    const uint16_t segment)
{
    struct range range = RANGE_EMPTY();
    if (!range_create_and_verify(base_addr,
                                 map_size_for_bus_range(bus_range),
                                 &range))
    {
        printk(LOGLEVEL_WARN,
               "pci: ecam domain at base %p, segment %" PRIu16 " overflows\n",
               (void *)base_addr,
               segment);

        return NULL;
    }

    struct pci_domain_ecam *const ecam_domain = kmalloc(sizeof(*ecam_domain));
    if (ecam_domain == NULL) {
        printk(LOGLEVEL_WARN, "pci: failed to alloc ecam domain info\n");
        return NULL;
    }

    ecam_domain->domain = PCI_DOMAIN_INIT(PCI_DOMAIN_ECAM, segment);
    ecam_domain->mmio = vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (ecam_domain->mmio == NULL) {
        kfree(ecam_domain);
        printk(LOGLEVEL_WARN, "pci/ecam: failed to mmio-map config-domain\n");

        return NULL;
    }

    list_init(&ecam_domain->list);
    ecam_domain->bus_range = bus_range;

    const int flag = spin_acquire_save_irq(&g_ecam_domain_lock);

    list_add(&g_ecam_entity_list, &ecam_domain->list);
    g_ecam_entity_count++;

    const bool result = pci_add_domain(&ecam_domain->domain);
    spin_release_restore_irq(&g_ecam_domain_lock, flag);

    if (!result) {
        vunmap_mmio(ecam_domain->mmio);
        kfree(ecam_domain);

        return NULL;
    }

    return ecam_domain;
}

__debug_optimize(3)
bool pci_remove_ecam_domain(struct pci_domain_ecam *const ecam_domain) {
    const int flag = spin_acquire_save_irq(&g_ecam_domain_lock);

    pci_remove_domain(&ecam_domain->domain);
    vunmap_mmio(ecam_domain->mmio);

    list_deinit(&ecam_domain->list);
    g_ecam_entity_count--;

    spin_release_restore_irq(&g_ecam_domain_lock, flag);
    kfree(ecam_domain);

    return true;
}

__debug_optimize(3) uint64_t
pci_ecam_domain_loc_get_offset(const struct pci_domain_ecam *const domain,
                               const struct pci_location *const loc)
{
    return
        range_index_for_loc(domain->bus_range, loc->bus) << 20
      | (uint64_t)loc->slot << 15
      | (uint64_t)loc->function << 12;
}

__debug_optimize(3) uint8_t
pci_ecam_read_8(const struct pci_domain_ecam *const domain,
                const struct pci_location *const loc,
                const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint8_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    return mmio_read_8(domain->mmio->base + offset);
}

__debug_optimize(3) uint16_t
pci_ecam_read_16(const struct pci_domain_ecam *const domain,
                 const struct pci_location *const loc,
                 const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint16_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    return mmio_read_16(domain->mmio->base + offset);
}

__debug_optimize(3) uint32_t
pci_ecam_read_32(const struct pci_domain_ecam *const domain,
                 const struct pci_location *const loc,
                 const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint32_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    return mmio_read_32(domain->mmio->base + offset);
}

__debug_optimize(3) uint64_t
pci_ecam_read_64(const struct pci_domain_ecam *const domain,
                 const struct pci_location *const loc,
                 const uint16_t off)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint64_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    return mmio_read_64(domain->mmio->base + offset);
}

__debug_optimize(3) void
pci_ecam_write_8(const struct pci_domain_ecam *const domain,
                 const struct pci_location *const loc,
                 const uint16_t off,
                 const uint8_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint8_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    mmio_write_8(domain->mmio->base + offset, value);
}

__debug_optimize(3) void
pci_ecam_write_16(const struct pci_domain_ecam *const domain,
                  const struct pci_location *const loc,
                  const uint16_t off,
                  const uint16_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint16_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    mmio_write_16(domain->mmio->base + offset, value);
}

__debug_optimize(3) void
pci_ecam_write_32(const struct pci_domain_ecam *const domain,
                  const struct pci_location *const loc,
                  const uint16_t off,
                  const uint32_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint32_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    mmio_write_32(domain->mmio->base + offset, value);
}

__debug_optimize(3) void
pci_ecam_write_64(const struct pci_domain_ecam *const domain,
                  const struct pci_location *const loc,
                  const uint16_t off,
                  const uint64_t value)
{
    assert(
        index_range_in_bounds(RANGE_INIT(off, sizeof(uint64_t)),
                              PCI_SPACE_MAX_OFFSET));

    const uint64_t offset = pci_ecam_domain_loc_get_offset(domain, loc) + off;
    mmio_write_64(domain->mmio->base + offset, value);
}

enum pci_ecam_dtb_range_kind {
    PCI_ECAM_DTB_RANGE_KIND_IO = 1,
    PCI_ECAM_DTB_RANGE_KIND_MEM32,
    PCI_ECAM_DTB_RANGE_KIND_MEM64,
};

enum pci_ecam_dtb_child_addr_shifts {
    PCI_ECAM_DTB_CHILD_ADDR_RNG_KIND_SHIFT = 24,
};

enum pci_ecam_dtb_child_addr_flags {
    __PCI_ECAM_DTB_CHILD_ADDR_RNG_KIND =
        0b11 << PCI_ECAM_DTB_CHILD_ADDR_RNG_KIND_SHIFT,

    __PCI_ECAM_DTB_CHILD_ADDR_IS_PREFETCH_MEM = 1 << 30
};

__debug_optimize(3) static inline bool
resource_kind_from_kind(const uint32_t flags,
                        enum pci_bus_resource_kind *const kind_out)
{
    const enum pci_ecam_dtb_range_kind kind =
        (flags & __PCI_ECAM_DTB_CHILD_ADDR_RNG_KIND) >>
            PCI_ECAM_DTB_CHILD_ADDR_RNG_KIND_SHIFT;

    switch (kind) {
        case PCI_ECAM_DTB_RANGE_KIND_IO:
            *kind_out = PCI_BUS_RESOURCE_IO;
            return true;
        case PCI_ECAM_DTB_RANGE_KIND_MEM32:
        case PCI_ECAM_DTB_RANGE_KIND_MEM64:
            if (flags & __PCI_ECAM_DTB_CHILD_ADDR_IS_PREFETCH_MEM) {
                *kind_out = PCI_BUS_RESOURCE_PREFETCH_MEM;
            } else {
                *kind_out = PCI_BUS_RESOURCE_MEM;
            }

            return true;
    }

    printk(LOGLEVEL_WARN, "pci/ecam: unrecognized range kind in flags\n");
    return false;
}

static bool
parse_dtb_resources(const struct devicetree_node *const node,
                    struct pci_bus *const root_bus)
{
    const struct devicetree_prop_ranges *const ranges_prop =
        (const struct devicetree_prop_ranges *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_RANGES);

    if (ranges_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "pci/ecam: 'ranges' property in dtb node is missing\n");
        return false;
    }

    if (!ranges_prop->has_flags) {
        printk(LOGLEVEL_WARN, "pci/ecam: 'ranges' prop is missing flags\n");
        return false;
    }

    uint32_t index = 0;
    array_foreach(&ranges_prop->list,
                  const struct devicetree_prop_range_info,
                  iter)
    {
        if (iter->size == 0) {
            printk(LOGLEVEL_WARN,
                   "pci/ecam: range #%" PRIu32 " in 'ranges' dtb-node prop has "
                   "a size of zero\n",
                   index + 1);
            continue;
        }

        struct range res_range = RANGE_EMPTY();
        if (!range_create_and_verify(iter->parent_bus_address,
                                     iter->size,
                                     &res_range))
        {
            printk(LOGLEVEL_WARN,
                   "pci/ecam: range #%" PRIu32 " in 'ranges' dtb-node prop "
                   "overflows\n",
                   index + 1);
            continue;
        }

        struct range res_mmio_range = RANGE_EMPTY();
        if (!range_align_out(res_range, PAGE_SIZE, &res_mmio_range)) {
            printk(LOGLEVEL_WARN,
                   "pci/ecam: can't align range #%" PRIu32 " in 'ranges' "
                   "dtb-node prop has a size of zero\n",
                   index);
            continue;
        }

        enum pci_bus_resource_kind res_kind;
        if (!resource_kind_from_kind(iter->flags, &res_kind)) {
            continue;
        }

        uint64_t flags = 0;
        if (res_kind == PCI_BUS_RESOURCE_PREFETCH_MEM) {
            flags |= __VMAP_MMIO_WT;
        }

        struct mmio_region *const mmio =
            vmap_mmio(res_mmio_range, PROT_READ | PROT_WRITE, flags);

        if (mmio == NULL) {
            printk(LOGLEVEL_WARN,
                   "pci/ecam: failed to mmio-map range #%" PRIu32 " in "
                   "'ranges' dtb-node prop has a size of zero\n",
                   index);
            continue;
        }

        struct pci_bus_resource resource =
            PCI_BUS_RESOURCE_INIT(res_kind,
                                  /*host_base=*/iter->parent_bus_address,
                                  /*child_base=*/iter->child_bus_address,
                                  iter->size,
                                  mmio,
                                  /*is_host_mmio=*/true);

        if (!array_append(&root_bus->resources, &resource)) {
            printk(LOGLEVEL_INFO,
                   "pci/ecam: failed to add resource to bus-list\n");
            return false;
        }

        index++;
    }

    return true;
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
               "pci/ecam: 'reg' property in dtb node is missing\n");
        return false;
    }

    if (array_empty(reg_prop->list)) {
        printk(LOGLEVEL_WARN,
               "pci/ecam: 'reg' property in dtb node is empty\n");
        return false;
    }

    const struct devicetree_prop_bus_range *const bus_range_prop =
        (const struct devicetree_prop_bus_range *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_PCI_BUS_RANGE);

    if (bus_range_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "pci/ecam: 'bus-range' property in dtb node is missing\n");
        return false;
    }

    const struct range bus_range = bus_range_prop->range;
    uint64_t bus_range_end = 0;

    if (!range_get_end(bus_range, &bus_range_end)) {
        printk(LOGLEVEL_WARN,
               "pci/ecam: 'bus-range' provided in dtb node overflows\n");
        return false;
    }

    if (bus_range_end > UINT8_MAX) {
        printk(LOGLEVEL_WARN,
               "pci/ecam: 'bus-range' provided in dtb node " RANGE_FMT " is "
               "too wide\n",
               RANGE_FMT_ARGS(bus_range));
        return false;
    }

    const struct devicetree_prop_reg_info *const mmio_reg =
        array_front(reg_prop->list);

    struct range mmio_range = RANGE_EMPTY();
    if (!range_create_and_verify(mmio_reg->address,
                                 mmio_reg->size,
                                 &mmio_range))
    {
        printk(LOGLEVEL_WARN, "pci/ecam: mmio range overflows\n");
        return false;
    }

    if (!range_has_index_range(mmio_range,
                               RANGE_INIT(bus_range.front,
                                          map_size_for_bus_range(bus_range))))
    {
        printk(LOGLEVEL_INFO,
               "pci/ecam: bus-range " RANGE_FMT " of dtb node can't fit in "
               "provided mmio-range " RANGE_FMT "\n",
               RANGE_FMT_ARGS(bus_range),
               RANGE_FMT_ARGS(mmio_range));
        return false;
    }

    struct pci_domain_ecam *const ecam_domain =
        pci_add_ecam_domain(bus_range, mmio_range.front, /*segment=*/0);
    struct pci_bus *const root_bus =
        pci_bus_create(&ecam_domain->domain, bus_range.front, /*segment=*/0);

    if (root_bus == NULL) {
        pci_remove_ecam_domain(ecam_domain);
        printk(LOGLEVEL_WARN,
               "pci/ecam: failed to create root-bus from dtb node\n");

        return false;
    }

    // FIXME: We don't do anything when parse_dtb_resources() fails
    parse_dtb_resources(node, root_bus);
    if (!pci_add_root_bus(root_bus)) {
        pci_remove_root_bus(root_bus);
        pci_remove_ecam_domain(ecam_domain);

        printk(LOGLEVEL_INFO, "pci/ecam: failed to add bus to root-bus list\n");
        return false;
    }

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
    .name = SV_STATIC("pci-ecam-driver"),
    .dtb = &dtb_driver,
    .pci = NULL
};