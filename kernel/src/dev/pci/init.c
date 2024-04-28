/*
 * kernel/src/dev/pci/init.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "dev/pci/legacy.h"
    #include "acpi/api.h"
#endif /* defined(__x86_64__) */

#include "lib/adt/bitset.h"
#include "cpu/isr.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/util.h"
#include "mm/kmalloc.h"

#include "structs.h"

static struct list g_entity_list = LIST_INIT(g_entity_list);

enum parse_bar_result {
    E_PARSE_BAR_OK,
    E_PARSE_BAR_IGNORE,

    E_PARSE_BAR_UNKNOWN_MEM_KIND,
    E_PARSE_BAR_NO_REG_FOR_UPPER32,

    E_PARSE_BAR_RANGE_OVERFLOW,
    E_PARSE_BAR_MMIO_MAP_FAIL
};

#define read_bar(dev, index) \
    pci_domain_read_32((dev)->bus->domain, \
                       &(dev)->loc, \
                       offsetof(struct pci_spec_entity_info, bar_list) + \
                         ((index) * sizeof(uint32_t)))

#define write_bar(dev, index, value) \
    pci_domain_write_32((dev)->bus->domain, \
                        &(dev)->loc, \
                        offsetof(struct pci_spec_entity_info, bar_list) + \
                         ((index) * sizeof(uint32_t)), \
                        (value))

enum pci_bar_masks {
    PCI_BAR_32B_ADDR_SIZE_MASK = 0xfffffff0,
    PCI_BAR_PORT_ADDR_SIZE_MASK = 0xfffffffc
};

__optimize(3) static enum parse_bar_result
pci_bar_parse_size(struct pci_entity_info *const dev,
                   struct pci_entity_bar_info *const info,
                   uint64_t base_addr,
                   const uint32_t bar_0_index,
                   const uint32_t bar_0_orig)
{
    /*
     * To get the size, we have to
     *   (1) read the BAR register and save the value.
     *   (2) write ~0 to the BAR register, and read the new value.
     *   (3) Restore the original value
     *
     * Since we operate on two registers for 64-bit, we have to perform this
     * procedure on both registers for 64-bit.
     */

    write_bar(dev, bar_0_index, UINT32_MAX);
    const uint32_t bar_0_size = read_bar(dev, bar_0_index);
    write_bar(dev, bar_0_index, bar_0_orig);

    uint64_t size = 0;
    if (info->is_64_bit) {
        const uint32_t bar_1_index = bar_0_index + 1;
        const uint32_t bar_1_orig = read_bar(dev, bar_1_index);

        write_bar(dev, bar_1_index, UINT32_MAX);
        const uint64_t size_high = read_bar(dev, bar_1_index);
        write_bar(dev, bar_1_index, bar_1_orig);

        size = size_high << 32 | (bar_0_size & PCI_BAR_32B_ADDR_SIZE_MASK);
        size = ~size + 1;
    } else {
        uint32_t size_low =
            info->is_mmio ?
                bar_0_size & PCI_BAR_32B_ADDR_SIZE_MASK :
                bar_0_size & PCI_BAR_PORT_ADDR_SIZE_MASK;

        size_low = ~size_low + 1;
        size = size_low;
    }

    if (size == 0) {
        return E_PARSE_BAR_IGNORE;
    }

    if (!range_create_and_verify(base_addr, size, &info->port_or_phys_range)) {
        return E_PARSE_BAR_RANGE_OVERFLOW;
    }

    info->is_present = true;
    return E_PARSE_BAR_OK;
}

// index_in is incremented assuming index += 1 will be called by the caller
static enum parse_bar_result
pci_parse_bar(struct pci_entity_info *const dev,
              uint8_t *const index_in,
              const bool is_bridge,
              struct pci_entity_bar_info *const bar)
{
    uint64_t base_addr = 0;

    const uint8_t bar_0_index = *index_in;
    const uint32_t bar_0 = read_bar(dev, bar_0_index);

    if (bar_0 & __PCI_DEVBAR_IO) {
        base_addr = bar_0 & PCI_BAR_PORT_ADDR_SIZE_MASK;
        return pci_bar_parse_size(dev, bar, base_addr, bar_0_index, bar_0);
    }

    base_addr = bar_0 & PCI_BAR_32B_ADDR_SIZE_MASK;
    const enum pci_spec_devbar_memspace_kind memory_kind =
        (bar_0 & __PCI_DEVBAR_MEMKIND_MASK) >> PCI_DEVBAR_MEMKIND_SHIFT;

    enum parse_bar_result result = E_PARSE_BAR_OK;
    if (memory_kind == PCI_DEVBAR_MEMSPACE_32B) {
        result = pci_bar_parse_size(dev, bar, base_addr, bar_0_index, bar_0);
        goto mmio_map;
    }

    if (memory_kind != PCI_DEVBAR_MEMSPACE_64B) {
        printk(LOGLEVEL_WARN,
               "pci: bar has reserved memory kind %d\n", memory_kind);
        return E_PARSE_BAR_UNKNOWN_MEM_KIND;
    }

    // Check if we even have another register left
    const uint8_t last_index =
        is_bridge ?
            (PCI_BAR_COUNT_FOR_BRIDGE - 1) : (PCI_BAR_COUNT_FOR_GENERAL - 1);

    if (bar_0_index == last_index) {
        printk(LOGLEVEL_INFO, "pci: 64-bit bar has no upper32 register\n");
        return E_PARSE_BAR_NO_REG_FOR_UPPER32;
    }

    base_addr |= (uint64_t)read_bar(dev, bar_0_index + 1) << 32;
    bar->is_64_bit = true;

    // Increment once more since we read another register
    *index_in += 1;
    result = pci_bar_parse_size(dev, bar, base_addr, bar_0_index, bar_0);

mmio_map:
    if (result != E_PARSE_BAR_OK) {
        bar->is_64_bit = false;
        return result;
    }

    bar->is_mmio = true;
    bar->is_prefetchable = bar_0 & __PCI_DEVBAR_PREFETCHABLE;

    return E_PARSE_BAR_OK;
}

#undef write_bar
#undef read_bar

static bool
validate_cap_offset(struct array *const prev_cap_offsets,
                    const uint8_t cap_offset)
{
    uint32_t struct_size = offsetof(struct pci_spec_entity_info, data);
    if (index_in_bounds(cap_offset, struct_size)) {
        printk(LOGLEVEL_INFO,
               "\t\tinvalid entity. pci capability offset points to "
               "within structure: 0x%" PRIx8 "\n",
               cap_offset);
        return false;
    }

    const struct range cap_range =
        RANGE_INIT(cap_offset, sizeof(struct pci_spec_capability));

    if (!range_has_index_range(rangeof_field(struct pci_spec_entity_info, data),
                               cap_range))
    {
        printk(LOGLEVEL_INFO,
               "\t\tinvalid entity. pci capability struct is outside entity's "
               "data range: " RANGE_FMT "\n",
               RANGE_FMT_ARGS(cap_range));
        return false;
    }

    array_foreach(prev_cap_offsets, const uint8_t, iter) {
        if (*iter == cap_offset) {
            printk(LOGLEVEL_WARN,
                   "\t\tcapability'e offset_to_next points to previously "
                   "visited capability\n");
            return false;
        }

        const struct range range =
            RANGE_INIT(*iter, sizeof(struct pci_spec_capability));

        if (range_has_loc(range, cap_offset)) {
            printk(LOGLEVEL_WARN,
                   "\t\tcapability'e offset_to_next points to within "
                   "previously visited capability\n");
            return false;
        }
    }

    return true;
}

static void pci_parse_capabilities(struct pci_entity_info *const entity) {
    if ((entity->status & __PCI_DEVSTATUS_CAPABILITIES) == 0) {
        printk(LOGLEVEL_INFO, "\t\thas no capabilities\n");
        return;
    }

    uint8_t cap_offset =
        pci_read(entity, struct pci_spec_entity_info, capabilities_offset);

    if (cap_offset == 0 || cap_offset == (uint8_t)PCI_READ_FAIL) {
        printk(LOGLEVEL_INFO,
               "\t\thas no capabilities, but pci-entity is marked as having "
               "some\n");
        return;
    }

    if (!range_has_index(rangeof_field(struct pci_spec_entity_info, data),
                         cap_offset))
    {
        printk(LOGLEVEL_INFO,
               "\t\tcapabilities point to within entity-info structure\n");
        return;
    }

    const enum isr_msi_support msi_support = isr_get_msi_support();

#define pci_read_cap_field(type, field) \
    pci_read_from_base(entity, cap_offset, type, field)

    for (uint64_t i = 0; cap_offset != 0 && cap_offset != 0xff; i++) {
        if (i == 128) {
            printk(LOGLEVEL_INFO,
                   "\t\ttoo many capabilities for "
                   "entity " PCI_ENTITY_INFO_FMT "\n",
                   PCI_ENTITY_INFO_FMT_ARGS(entity));
            return;
        }

        if (!validate_cap_offset(&entity->vendor_cap_list, cap_offset)) {
            continue;
        }

        const uint8_t id = pci_read_cap_field(struct pci_spec_capability, id);
        const char *kind = "unknown";

        switch ((enum pci_spec_cap_id)id) {
            case PCI_SPEC_CAP_ID_NULL:
                kind = "null";
                break;
            case PCI_SPEC_CAP_ID_AGP:
                kind = "advanced-graphics-port";
                break;
            case PCI_SPEC_CAP_ID_VPD:
                kind = "vital-product-data";
                break;
            case PCI_SPEC_CAP_ID_SLOT_ID:
                kind = "slot-identification";
                break;
            case PCI_SPEC_CAP_ID_MSI: {
                kind = "msi";

                const struct range msi_range =
                    RANGE_INIT(cap_offset, sizeof(struct pci_spec_cap_msi));

                if (!index_range_in_bounds(msi_range, PCI_SPACE_MAX_OFFSET)) {
                    printk(LOGLEVEL_WARN,
                           "\t\tmsi-cap goes beyond end of pci-domain\n");
                    break;
                }

                if (msi_support == ISR_MSI_SUPPORT_NONE) {
                    break;
                }

                uint16_t msg_control =
                    pci_read_cap_field(struct pci_spec_cap_msi, msg_control);

                if (msg_control & __PCI_CAP_MSI_CTRL_ENABLE
                    || msg_control & __PCI_CAP_MSI_CTRL_MULTIMSG_ENABLE)
                {
                    msg_control =
                        rm_mask(msg_control,
                                __PCI_CAP_MSI_CTRL_ENABLE
                                | __PCI_CAP_MSI_CTRL_MULTIMSG_ENABLE);

                    pci_write_from_base(entity,
                                        entity->msi_pcie_offset,
                                        struct pci_spec_cap_msi,
                                        msg_control,
                                        msg_control);
                }

                if (entity->msi_support == PCI_ENTITY_MSI_SUPPORT_MSIX) {
                    break;
                }

                entity->msi_support = PCI_ENTITY_MSI_SUPPORT_MSI;
                entity->msi_pcie_offset = cap_offset;

                entity->msi.enabled = false;
                entity->msi.supports_64bit =
                    msg_control & __PCI_CAP_MSI_CTRL_64BIT_CAPABLE;
                entity->msi.supports_masking =
                    msg_control & __PCI_CAP_MSI_CTRL_PER_VECTOR_MASK;

                break;
            }
            case PCI_SPEC_CAP_ID_MSI_X: {
                kind = "msix";
                if (entity->msi_support == PCI_ENTITY_MSI_SUPPORT_MSIX) {
                    printk(LOGLEVEL_WARN,
                           "\t\tfound multiple msix capabilities. ignoring\n");
                    break;
                }

                const struct range msix_range =
                    RANGE_INIT(cap_offset, sizeof(struct pci_spec_cap_msix));

                if (!index_range_in_bounds(msix_range, PCI_SPACE_MAX_OFFSET)) {
                    printk(LOGLEVEL_WARN,
                           "\t\tmsix-cap goes beyond end of pci-domain\n");
                    break;
                }

                if (msi_support != ISR_MSI_SUPPORT_MSIX) {
                    break;
                }

                uint16_t msg_control =
                    pci_read_cap_field(struct pci_spec_cap_msix, msg_control);

                if (msg_control & __PCI_CAP_MSIX_CTRL_ENABLE
                    || msg_control & __PCI_CAP_MSIX_CTRL_FUNC_MASK)
                {
                    msg_control =
                        rm_mask(msg_control,
                                __PCI_CAP_MSIX_CTRL_ENABLE
                                | __PCI_CAP_MSIX_CTRL_FUNC_MASK);

                    pci_write_from_base(entity,
                                        entity->msi_pcie_offset,
                                        struct pci_spec_cap_msix,
                                        msg_control,
                                        msg_control);
                }

                entity->msi_pcie_offset = cap_offset;
                entity->msi_support = PCI_ENTITY_MSI_SUPPORT_MSIX;
                entity->msix.table_size =
                    (msg_control & __PCI_CAP_MSIX_CTRL_TABLE_SIZE) + 1;

                break;
            }
            case PCI_SPEC_CAP_ID_POWER_MANAGEMENT:
                kind = "power-management";
                break;
            case PCI_SPEC_CAP_ID_VENDOR_SPECIFIC:
                if (!array_append(&entity->vendor_cap_list, &cap_offset)) {
                    printk(LOGLEVEL_WARN,
                           "\t\tfailed to append to internal array\n");
                    return;
                }

                kind = "vendor-specific";
                break;
            case PCI_SPEC_CAP_ID_PCI_X:
                kind = "pci-x";
                break;
            case PCI_SPEC_CAP_ID_DEBUG_PORT:
                kind = "debug-port";
                break;
            case PCI_SPEC_CAP_ID_SATA:
                kind = "sata";
                break;
            case PCI_SPEC_CAP_ID_COMPACT_PCI_HOT_SWAP:
                kind = "compact-pci-hot-swap";
                break;
            case PCI_SPEC_CAP_ID_HYPER_TRANSPORT:
                kind = "hyper-transport";
                break;
            case PCI_SPEC_CAP_ID_COMPACT_PCI_CENTRAL_RSRC_CNTRL:
                kind = "compact-pci-central-resource-control";
                break;
            case PCI_SPEC_CAP_ID_PCI_HOTPLUG:
                kind = "pci-hotplug";
                break;
            case PCI_SPEC_CAP_ID_PCI_BRIDGE_SYS_VENDOR_ID:
                kind = "pci-bridge-system-vendor-id";
                break;
            case PCI_SPEC_CAP_ID_AGP_8X:
                kind = "advanced-graphics-port-8x";
                break;
            case PCI_SPEC_CAP_ID_SECURE_DEVICE:
                kind = "secure-device";
                break;
            case PCI_SPEC_CAP_ID_PCI_EXPRESS: {
                kind = "pci-express";
                const struct range pcie_range =
                    RANGE_INIT(cap_offset, sizeof(struct pci_spec_cap_pcie));

                if (!index_range_in_bounds(pcie_range, PCI_SPACE_MAX_OFFSET)) {
                    printk(LOGLEVEL_WARN,
                           "\t\tpcie-cap goes beyond end of pci-domain\n");
                    break;
                }

                entity->supports_pcie = true;
                break;
            }
            case PCI_SPEC_CAP_ID_ADV_FEATURES:
                kind = "advanced-features";
                break;
            case PCI_SPEC_CAP_ID_ENHANCED_ALLOCS:
                kind = "enhanced-allocations";
                break;
            case PCI_SPEC_CAP_ID_FLAT_PORTAL_BRIDGE:
                kind = "flattening-portal-bridge";
                break;
        }

        printk(LOGLEVEL_INFO,
               "\t\tfound capability: %s at offset 0x%" PRIx8 "\n",
               kind,
               cap_offset);

        cap_offset =
            pci_read_cap_field(struct pci_spec_capability, offset_to_next);
    }

#undef pci_read_cap_field
}

__optimize(3) static inline
const char *pci_entity_get_vendor_name(struct pci_entity_info *const entity) {
    carr_foreach(pci_vendor_info_list, iter) {
        if (entity->vendor_id == iter->id) {
            return iter->name;
        }
    }

    return "unknown";
}

void
pci_parse_bus(struct pci_bus *bus, struct pci_location loc, uint8_t bus_id);

static void
parse_function(struct pci_bus *const bus,
               const struct pci_location *const loc,
               const uint16_t vendor_id)
{
    struct pci_entity_info *const entity = kmalloc(sizeof(*entity));
    if (entity == NULL) {
        printk(LOGLEVEL_WARN,
               "pci: failed to allocate pci-entity-info struct\n");

        return;
    }

    entity->bus = bus;
    entity->loc = *loc;
    entity->lock = SPINLOCK_INIT();

    list_init(&entity->list_in_entities);
    list_init(&entity->list_in_domain);

    const uint8_t header_kind =
        pci_read(entity, struct pci_spec_entity_info_base, header_kind);

    const uint8_t hdrkind = rm_mask(header_kind, __PCI_ENTITY_HDR_MULTFUNC);
    const uint8_t irq_pin =
        hdrkind == PCI_SPEC_ENTITY_HDR_KIND_GENERAL ?
            pci_read(entity, struct pci_spec_entity_info, interrupt_pin) : 0;

    const uint8_t irq_line =
        hdrkind == PCI_SPEC_ENTITY_HDR_KIND_GENERAL ?
            pci_read(entity, struct pci_spec_entity_info, interrupt_line) : 0;

    entity->id = pci_read(entity, struct pci_spec_entity_info_base, device_id);
    entity->vendor_id = vendor_id;
    entity->status = pci_read(entity, struct pci_spec_entity_info_base, status);
    entity->revision_id =
        pci_read(entity, struct pci_spec_entity_info_base, revision_id);
    entity->prog_if =
        pci_read(entity, struct pci_spec_entity_info_base, prog_if);

    entity->header_kind = hdrkind;
    entity->class =
        pci_read(entity, struct pci_spec_entity_info_base, class_code);
    entity->subclass =
        pci_read(entity, struct pci_spec_entity_info_base, subclass);
    entity->interrupt_pin = irq_pin;
    entity->interrupt_line = irq_line;
    entity->vendor_cap_list = ARRAY_INIT(sizeof(uint8_t));

    printk(LOGLEVEL_INFO,
           "\tentity: " PCI_ENTITY_INFO_FMT " from %s\n",
           PCI_ENTITY_INFO_FMT_ARGS(entity),
           pci_entity_get_vendor_name(entity));

    const bool class_is_pci_bridge =
        entity->class == PCI_ENTITY_CLASS_BRIDGE_DEVICE
        && (entity->subclass == PCI_ENTITY_SUBCLASS_PCI_BRIDGE
            || entity->subclass == PCI_ENTITY_SUBCLASS_PCI_BRIDGE_2);

    const bool hdrkind_is_pci_bridge =
        hdrkind == PCI_SPEC_ENTITY_HDR_KIND_PCI_BRIDGE;

    if (hdrkind_is_pci_bridge != class_is_pci_bridge) {
        pci_entity_info_destroy(entity);
        printk(LOGLEVEL_WARN,
               "pci: invalid entity, header-type and class/subclass "
               "mismatch\n");

        return;
    }

    // Disable I/O Space and Memory domain flags to parse bars.
    const uint16_t disable_flags =
        __PCI_DEVCMDREG_IOSPACE
        | __PCI_DEVCMDREG_MEMSPACE
        | __PCI_DEVCMDREG_BUS_MASTER;

    const uint16_t old_command =
        pci_read(entity, struct pci_spec_entity_info_base, command);
    const uint16_t new_command =
        rm_mask(old_command, disable_flags) | __PCI_DEVCMDREG_PIN_INT_DISABLE;

    pci_write(entity, struct pci_spec_entity_info_base, command, new_command);
    switch (hdrkind) {
        case PCI_SPEC_ENTITY_HDR_KIND_GENERAL:
            entity->max_bar_count = PCI_BAR_COUNT_FOR_GENERAL;
            entity->bar_list =
                kmalloc(sizeof(struct pci_entity_bar_info)
                        * entity->max_bar_count);

            if (entity->bar_list == NULL) {
                pci_entity_info_destroy(entity);
                printk(LOGLEVEL_WARN,
                       "pci: failed to allocate memory for bar list\n");

                return;
            }

            pci_parse_capabilities(entity);
            for (uint8_t index = 0; index != entity->max_bar_count; index++) {
                struct pci_entity_bar_info *const bar = &entity->bar_list[index];
                const uint8_t bar_index = index;

                const enum parse_bar_result result =
                    pci_parse_bar(entity, &index, /*is_bridge=*/false, bar);

                if (result == E_PARSE_BAR_IGNORE) {
                    printk(LOGLEVEL_INFO,
                           "\t\tgeneral bar %" PRIu8 ": ignoring\n",
                           index);

                    bar->is_present = false;
                    continue;
                }

                if (result != E_PARSE_BAR_OK) {
                    printk(LOGLEVEL_WARN,
                           "pci: failed to parse bar %" PRIu8 " for entity\n",
                           index);

                    bar->is_present = false;
                    break;
                }

                printk(LOGLEVEL_INFO,
                       "\t\tgeneral bar %" PRIu8 " %s: " RANGE_FMT ", %s, %s"
                       "size: %" PRIu64 "\n",
                       bar_index,
                       bar->is_mmio ? "mmio" : "ports",
                       RANGE_FMT_ARGS(bar->port_or_phys_range),
                       bar->is_prefetchable ?
                        "prefetchable" : "not-prefetchable",
                       bar->is_mmio ?
                        bar->is_64_bit ? "64-bit, " : "32-bit, "
                        : "",
                       bar->port_or_phys_range.size);
            }

            break;
        case PCI_SPEC_ENTITY_HDR_KIND_PCI_BRIDGE:
            entity->max_bar_count = PCI_BAR_COUNT_FOR_BRIDGE;
            entity->bar_list =
                kmalloc(sizeof(struct pci_entity_bar_info)
                        * entity->max_bar_count);

            if (entity->bar_list == NULL) {
                pci_entity_info_destroy(entity);
                printk(LOGLEVEL_WARN,
                       "pci: failed to allocate memory for bar list\n");

                return;
            }

            pci_parse_capabilities(entity);
            for (uint8_t index = 0; index != entity->max_bar_count; index++) {
                struct pci_entity_bar_info *const bar =
                    &entity->bar_list[index];

                const uint8_t bar_index = index;
                const enum parse_bar_result result =
                    pci_parse_bar(entity, &index, /*is_bridge=*/true, bar);

                if (result == E_PARSE_BAR_IGNORE) {
                    printk(LOGLEVEL_INFO,
                           "\t\tbridge bar %" PRIu8 ": ignoring\n",
                           index);
                    continue;
                }

                if (result != E_PARSE_BAR_OK) {
                    printk(LOGLEVEL_INFO,
                           "pci: failed to parse bar %" PRIu8 " for "
                           "entity, " PCI_ENTITY_INFO_FMT "\n",
                           index,
                           PCI_ENTITY_INFO_FMT_ARGS(entity));
                    break;
                }

                printk(LOGLEVEL_INFO,
                       "\t\tbridge bar %" PRIu8 " %s: " RANGE_FMT ", %s, %s"
                       "size: %" PRIu64 "\n",
                       bar_index,
                       bar->is_mmio ? "mmio" : "ports",
                       RANGE_FMT_ARGS(bar->port_or_phys_range),
                       bar->is_prefetchable ?
                        "prefetchable" : "not-prefetchable",
                       bar->is_mmio ?
                        bar->is_64_bit ? "64-bit, " : "32-bit, "
                        : "",
                       bar->port_or_phys_range.size);
            }

            const uint8_t secondary_bus_number =
                pci_read(entity,
                         struct pci_spec_pci_to_pci_bridge_entity_info,
                         secondary_bus_number);

            pci_parse_bus(bus, entity->loc, secondary_bus_number);
            break;
        case PCI_SPEC_ENTITY_HDR_KIND_CARDBUS_BRIDGE:
            pci_entity_info_destroy(entity);
            printk(LOGLEVEL_INFO,
                   "pcie: cardbus bridge not supported. ignoring");

            return;
    }

    if (entity->msi_support == PCI_ENTITY_MSI_SUPPORT_MSIX) {
        const uint32_t table_offset =
            pci_read_from_base(entity,
                               entity->msi_pcie_offset,
                               struct pci_spec_cap_msix,
                               table_offset);

        const uint8_t bar_index =
            table_offset & __PCI_BARSPEC_TABLE_OFFSET_BIR;

        if (!index_in_bounds(bar_index, entity->max_bar_count)) {
            pci_entity_info_destroy(entity);
            printk(LOGLEVEL_WARN,
                    "pcie: invalid msix table-bar index %" PRIu32 "\n",
                    bar_index);

            return;
        }

        struct pci_entity_bar_info *const bar =
            &entity->bar_list[bar_index];

        if (!bar->is_present) {
            pci_entity_info_destroy(entity);
            printk(LOGLEVEL_WARN, "pcie: msix table-bar is not present\n");

            return;
        }

        if (!bar->is_mmio) {
            pci_entity_info_destroy(entity);
            printk(LOGLEVEL_WARN, "pcie: msix table-bar is not mmio\n");

            return;
        }

        uint64_t *const bitset =
            kmalloc(bitset_size_for_count(entity->msix.table_size));

        if (bitset == NULL) {
            pci_entity_info_destroy(entity);
            printk(LOGLEVEL_WARN, "pcie: failed to alloc msix table bitset\n");

            return;
        }

        entity->msix.table_bar = bar;
        entity->msix.bitset = bitset;
        entity->msix.table_offset =
            rm_mask(table_offset, __PCI_BARSPEC_TABLE_OFFSET_BIR);
    }

    list_add(&g_entity_list, &entity->list_in_entities);
    list_add(&bus->entity_list, &entity->list_in_domain);
}

void
pci_parse_bus(struct pci_bus *const bus,
              struct pci_location loc,
              const uint8_t bus_id)
{
    loc.bus += bus_id;
    for (uint8_t slot = 0; slot != PCI_MAX_SLOT_COUNT; slot++) {
        loc.slot = slot;
        for (uint8_t func = 0; func != PCI_MAX_FUNCTION_COUNT; func++) {
            loc.function = func;
            const uint16_t vendor_id =
                pci_domain_read_16(
                    bus->domain,
                    &loc,
                    offsetof(struct pci_spec_entity_info_base, vendor_id));

            if (vendor_id == (uint16_t)PCI_READ_FAIL) {
                continue;
            }

            parse_function(bus, &loc, vendor_id);
        }
    }
}

void pci_find_entities(struct pci_bus *const bus) {
    struct pci_location loc = {
        .segment = bus->segment,
        .bus = bus->bus_id,
        .slot = 0,
        .function = 0,
    };

    const uint8_t header_kind =
        pci_domain_read_8(bus->domain,
                          &loc,
                          offsetof(struct pci_spec_entity_info_base,
                                   header_kind));

    if (header_kind == (uint8_t)PCI_READ_FAIL) {
        return;
    }

    const uint8_t host_count =
        header_kind & __PCI_ENTITY_HDR_MULTFUNC ? PCI_MAX_FUNCTION_COUNT : 1;

    for (uint16_t i = 0; i != host_count; i++) {
        pci_parse_bus(bus, loc, /*bus=*/i);
    }
}

void pci_init_drivers() {
    driver_foreach(driver) {
        if (driver->pci == NULL) {
            continue;
        }

        const struct pci_driver *const pci_driver = driver->pci;
        struct pci_entity_info *entity = NULL;

        list_foreach(entity, &g_entity_list, list_in_entities) {
            if (pci_driver->match == PCI_DRIVER_MATCH_VENDOR) {
                if (entity->vendor_id == pci_driver->vendor) {
                    pci_driver->init(entity);
                }

                continue;
            }

            if (pci_driver->match == PCI_DRIVER_MATCH_VENDOR_DEVICE) {
                if (entity->vendor_id != pci_driver->vendor) {
                    continue;
                }

                bool found = false;
                for (uint8_t i = 0; i != pci_driver->device_count; i++) {
                    if (pci_driver->devices[i] == entity->id) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    pci_driver->init(entity);
                }

                continue;
            }

            if (pci_driver->match & __PCI_DRIVER_MATCH_CLASS) {
                if (entity->class != pci_driver->class) {
                    continue;
                }
            }

            if (pci_driver->match & __PCI_DRIVER_MATCH_SUBCLASS) {
                if (entity->subclass != pci_driver->subclass) {
                    continue;
                }
            }

            if (pci_driver->match & __PCI_DRIVER_MATCH_PROGIF) {
                if (entity->prog_if != pci_driver->prog_if) {
                    continue;
                }
            }

            pci_driver->init(entity);
        }
    }
}

__optimize(3) void pci_init() {
    int flag = 0;
    const struct array *const bus_list = pci_get_root_bus_list_locked(&flag);

#if defined(__x86_64__)
    if (array_empty(*bus_list)) {
        pci_release_root_bus_list_lock(flag);
        printk(LOGLEVEL_INFO,
               "pci: searching for entities in root bus (legacy domain)\n");

        const struct pci_location loc = {
            .segment = 0,
            .bus = 0,
            .slot = 0,
            .function = 0
        };

        const uint32_t first_dword =
            pci_legacy_domain_read(
                &loc,
                offsetof(struct pci_spec_entity_info_base, vendor_id),
                sizeof(uint16_t));

        if (first_dword == PCI_READ_FAIL) {
            printk(LOGLEVEL_WARN,
                   "pci: failed to find pci bus in legacy domain. aborting "
                   "init\n");
            return;
        }

        struct pci_domain *const legacy_domain =
            kmalloc(sizeof(*legacy_domain));

        assert_msg(legacy_domain != NULL,
                   "pci: failed to allocate pci-legacy root domain");

        legacy_domain->kind = PCI_DOMAIN_LEGACY;
        struct pci_bus *const root_bus =
            pci_bus_create(legacy_domain, /*bus_id=*/0, /*segment=*/0);

        assert_msg(root_bus != NULL,
                   "pci: failed to allocate pci-legacy root bus");

        pci_find_entities(root_bus);
        pci_add_root_bus(root_bus);
    } else {
        printk(LOGLEVEL_INFO, "pci: no root-bus found. Aborting init\n");
    }
#else
    if (!array_empty(*bus_list)) {
        printk(LOGLEVEL_INFO,
                "pci: searching for entities in every pci-bus\n");

        array_foreach(bus_list, struct pci_bus *const, iter) {
            pci_find_entities(*iter);
        }
    } else {
        printk(LOGLEVEL_INFO, "pci: no root-bus found. Aborting init\n");
        pci_release_root_bus_list_lock(flag);
    }
#endif /* defined(__x86_64__) */

    pci_init_drivers();
}
