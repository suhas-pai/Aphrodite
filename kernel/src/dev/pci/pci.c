/*
 * kernel/dev/pci/pci.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/mmio.h"
#include "sys/port.h"

#include "structs.h"

static struct list g_domain_list = LIST_INIT(g_domain_list);
static struct list g_device_list = LIST_INIT(g_device_list);

static uint64_t g_domain_count = 0;

enum parse_bar_result {
    E_PARSE_BAR_OK,
    E_PARSE_BAR_IGNORE,

    E_PARSE_BAR_UNKNOWN_MEM_KIND,
    E_PARSE_BAR_NO_REG_FOR_UPPER32,

    E_PARSE_BAR_MMIO_MAP_FAIL
};

#define read_bar(index) \
    arch_pci_read(dev,  \
                  offsetof(struct pci_spec_device_info, bar_list) + \
                      ((index) * sizeof(uint32_t)),                 \
                  sizeof(uint32_t))

#define write_bar(index, value) \
    arch_pci_write(dev,         \
                   offsetof(struct pci_spec_device_info, bar_list) + \
                       ((index) * sizeof(uint32_t)), \
                   (value),                          \
                   sizeof(uint32_t))

enum pci_bar_masks {
    PCI_BAR_32B_ADDR_SIZE_MASK = 0xfffffff0,
    PCI_BAR_PORT_ADDR_SIZE_MASK = 0xfffffffc
};

static enum parse_bar_result
pci_bar_parse_size(struct pci_device_info *const dev,
                   struct pci_device_bar_info *const info,
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

    write_bar(bar_0_index, UINT32_MAX);
    const uint32_t bar_0_size = read_bar(bar_0_index);
    write_bar(bar_0_index, bar_0_orig);

    uint64_t size = 0;
    if (info->is_64_bit) {
        const uint32_t bar_1_index = bar_0_index + 1;
        const uint32_t bar_1_orig = read_bar(bar_1_index);

        write_bar(bar_1_index, UINT32_MAX);
        const uint64_t size_high = read_bar(bar_1_index);
        write_bar(bar_1_index, bar_1_orig);

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

    // We use port range to temporarily store the phys range.
    info->port_or_phys_range = RANGE_INIT(base_addr, size);
    info->is_present = true;

    return E_PARSE_BAR_OK;
}

// index_in is incremented assuming index += 1 will be called by the caller
static enum parse_bar_result
pci_parse_bar(struct pci_device_info *const dev,
              uint8_t *const index_in,
              const bool is_bridge,
              struct pci_device_bar_info *const bar)
{
    uint64_t base_addr = 0;

    const uint8_t bar_0_index = *index_in;
    const uint32_t bar_0 = read_bar(bar_0_index);

    if (bar_0 & __PCI_DEVBAR_IO) {
        base_addr = bar_0 & PCI_BAR_PORT_ADDR_SIZE_MASK;
        return pci_bar_parse_size(dev, bar, base_addr, bar_0_index, bar_0);
    }

    base_addr = bar_0 & PCI_BAR_32B_ADDR_SIZE_MASK;
    const enum pci_spec_devbar_memspace_kind memory_kind =
        (bar_0 & __PCI_DEVBAR_MEMKIND_MASK) >> __PCI_DEVBAR_MEMKIND_SHIFT;

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

    base_addr |= (uint64_t)read_bar(bar_0_index + 1) << 32;
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

bool pci_map_bar(struct pci_device_bar_info *const bar) {
    if (!bar->is_mmio) {
        printk(LOGLEVEL_WARN, "pcie: pci_map_bar() called on non-mmio bar\n");
        return false;
    }

    if (bar->is_mapped) {
        return true;
    }

    uint64_t flags = 0;
    if (bar->is_prefetchable) {
        flags |= __VMAP_MMIO_WT;
    }

    // We use port_range to internally store the phys range.
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
        return E_PARSE_BAR_MMIO_MAP_FAIL;
    }

    const uint64_t index_in_map = phys_range.front - aligned_range.front;

    bar->mmio = mmio;
    bar->index_in_mmio = index_in_map;
    bar->is_mapped = true;

    return true;
}

uint8_t
pci_device_bar_read8(struct pci_device_bar_info *const bar,
                     const uint32_t offset)
{
    if (!bar->is_mapped) {
        return UINT8_MAX;
    }

    return
        bar->is_mmio ?
            mmio_read_8(bar->mmio->base + bar->index_in_mmio + offset) :
            port_in8((port_t)(bar->port_or_phys_range.front + offset));
}

uint16_t
pci_device_bar_read16(struct pci_device_bar_info *const bar,
                      const uint32_t offset)
{
    if (!bar->is_mapped) {
        return UINT16_MAX;
    }

    return
        bar->is_mmio ?
            mmio_read_16(bar->mmio->base + bar->index_in_mmio + offset) :
            port_in16((port_t)(bar->port_or_phys_range.front + offset));
}

uint32_t
pci_device_bar_read32(struct pci_device_bar_info *const bar,
                      const uint32_t offset)
{
    if (!bar->is_mapped) {
        return UINT32_MAX;
    }

    return
        bar->is_mmio ?
            mmio_read_32(bar->mmio->base + bar->index_in_mmio + offset) :
            port_in32((port_t)(bar->port_or_phys_range.front + offset));
}

uint64_t
pci_device_bar_read64(struct pci_device_bar_info *const bar,
                      const uint32_t offset)
{
#if defined(__x86_64__)
    assert(bar->is_mmio);
    if (!bar->is_mapped) {
        return UINT64_MAX;
    }

    return mmio_read_64(bar->mmio->base + bar->index_in_mmio + offset);
#else
    if (!bar->is_mapped) {
        return UINT64_MAX;
    }

    return
        (bar->is_mmio) ?
            mmio_read_64(bar->mmio->base + bar->index_in_mmio + offset) :
            port_in64((port_t)(bar->port_or_phys_range.front + offset));
#endif /* defined(__x86_64__) */
}

static bool
validate_cap_offset(struct array *const prev_cap_offsets,
                    const uint8_t cap_offset)
{
    uint32_t struct_size = offsetof(struct pci_spec_device_info, data);
    if (index_in_bounds(cap_offset, struct_size)) {
        printk(LOGLEVEL_INFO,
               "\t\tinvalid device. pci capability offset points to "
               "within structure: 0x%" PRIx8 "\n",
               cap_offset);
        return false;
    }

    const struct range cap_range =
        RANGE_INIT(cap_offset, sizeof(struct pci_spec_capability));

    if (!range_has_index_range(rangeof_field(struct pci_spec_device_info, data),
                               cap_range))
    {
        printk(LOGLEVEL_INFO,
               "\t\tinvalid device. pci capability struct is outside device's "
               "data range: " RANGE_FMT "\n",
               RANGE_FMT_ARGS(cap_range));
        return false;
    }

    array_foreach(prev_cap_offsets, uint8_t, iter) {
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

static void pci_parse_capabilities(struct pci_device_info *const dev) {
    if ((dev->status & __PCI_DEVSTATUS_CAPABILITIES) == 0) {
        printk(LOGLEVEL_INFO, "\t\thas no capabilities\n");
        return;
    }

    uint8_t cap_offset =
        pci_read(dev, struct pci_spec_device_info, capabilities_offset);

    if (cap_offset == 0 || cap_offset == (uint8_t)PCI_READ_FAIL) {
        printk(LOGLEVEL_INFO,
               "\t\thas no capabilities, but pci-device is marked as having "
               "some\n");
        return;
    }

    if (!range_has_index(rangeof_field(struct pci_spec_device_info, data),
                         cap_offset))
    {
        printk(LOGLEVEL_INFO,
               "\t\tcapabilities point to within device-info structure\n");
        return;
    }

    // On x86_64, the fadt may provide a flag indicating the MSI is disabled.
#if defined(__x86_64__)
    bool supports_msi = true;
    const struct acpi_fadt *const fadt = get_acpi_info()->fadt;

    if (fadt != NULL) {
        if (fadt->iapc_boot_arch_flags &
                __ACPI_FADT_IAPC_BOOT_MSI_NOT_SUPPORTED)
        {
            supports_msi = false;
        }
    }
#endif

#define pci_read_cap_field(type, field) \
    pci_read_with_offset(dev, cap_offset, type, field)

    for (uint64_t i = 0; cap_offset != 0 && cap_offset != 0xff; i++) {
        if (i == 128) {
            printk(LOGLEVEL_INFO,
                   "\t\ttoo many capabilities for "
                   "device " PCI_DEVICE_INFO_FMT "\n",
                   PCI_DEVICE_INFO_FMT_ARGS(dev));
            return;
        }

        if (!validate_cap_offset(&dev->vendor_cap_list, cap_offset)) {
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
            case PCI_SPEC_CAP_ID_MSI:
                kind = "msi";
            #if defined(__x86_64__)
                if (!supports_msi) {
                    break;
                }
            #endif /* defined(__x86_64) */

                if (dev->msi_support == PCI_DEVICE_MSI_SUPPORT_MSIX) {
                    break;
                }

                dev->msi_support = PCI_DEVICE_MSI_SUPPORT_MSI;
                dev->pcie_msi_offset = cap_offset;

                break;
            case PCI_SPEC_CAP_ID_MSI_X:
                kind = "msix";
            #if defined(__x86_64__)
                if (!supports_msi) {
                    break;
                }
            #endif /* defined(__x86_64) */

                if (dev->msi_support == PCI_DEVICE_MSI_SUPPORT_MSIX) {
                    printk(LOGLEVEL_WARN,
                           "\t\tfound multiple msix capabilities. ignoring\n");
                    break;
                }

                dev->pcie_msix_offset = cap_offset;

                const uint16_t msg_control =
                    pci_read_cap_field(struct pci_spec_cap_msix, msg_control);
                const uint16_t bitmap_size =
                    (msg_control & __PCI_CAP_MSIX_TABLE_SIZE_MASK) + 1;

                const struct bitmap bitmap = bitmap_alloc(bitmap_size);
                if (bitmap.gbuffer.begin == NULL) {
                    printk(LOGLEVEL_WARN,
                           "\t\tfailed to allocate msix table "
                           "(size: %" PRIu16 " bytes). disabling msix\n",
                           bitmap_size);
                    break;
                }

                dev->msi_support = PCI_DEVICE_MSI_SUPPORT_MSIX;
                dev->msix_table = bitmap;

                break;
            case PCI_SPEC_CAP_ID_POWER_MANAGEMENT:
                kind = "power-management";
                break;
            case PCI_SPEC_CAP_ID_VENDOR_SPECIFIC:
                if (!array_append(&dev->vendor_cap_list, &cap_offset)) {
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
            case PCI_SPEC_CAP_ID_PCI_EXPRESS:
                dev->supports_pcie = true;
                kind = "pci-express";

                break;
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

static void free_inside_device_info(struct pci_device_info *const device) {
    struct pci_device_bar_info *const bar_list = device->bar_list;
    const uint8_t bar_count =
        device->header_kind == PCI_SPEC_DEVHDR_KIND_PCI_BRIDGE  ?
            PCI_BAR_COUNT_FOR_BRIDGE : PCI_BAR_COUNT_FOR_GENERAL;

    for (uint8_t i = 0; i != bar_count; i++) {
        struct pci_device_bar_info *const bar = &bar_list[i];
        if (bar->is_mapped) {
            vunmap_mmio(bar->mmio);
        }
    }

    if (device->msi_support == PCI_DEVICE_MSI_SUPPORT_MSIX) {
        bitmap_destroy(&device->msix_table);
    }

    array_destroy(&device->vendor_cap_list);
}

void
pci_parse_bus(struct pci_domain *domain,
              struct pci_config_space *config_space,
              uint8_t bus);

static void
parse_function(struct pci_domain *const domain,
               const struct pci_config_space *const config_space)
{
    struct pci_device_info info = {
        .domain = domain,
        .config_space = *config_space
    };

    if (domain->mmio->base != NULL) {
        info.pcie_info = domain->mmio->base + pci_device_get_index(&info);
    }

    const uint16_t vendor_id =
        pci_read(&info, struct pci_spec_device_info_base, vendor_id);

    if (vendor_id == (uint16_t)PCI_READ_FAIL) {
        return;
    }

    const uint8_t header_kind =
        pci_read(&info, struct pci_spec_device_info_base, header_kind);

    const bool is_multi_function = (header_kind & __PCI_DEVHDR_MULTFUNC) != 0;

    const uint8_t hdrkind = header_kind & (uint8_t)~__PCI_DEVHDR_MULTFUNC;
    const uint8_t irq_pin =
        hdrkind == PCI_SPEC_DEVHDR_KIND_GENERAL ?
            pci_read(&info, struct pci_spec_device_info, interrupt_pin) : 0;

    array_init(&info.vendor_cap_list, sizeof(uint8_t));

    info.id = pci_read(&info, struct pci_spec_device_info_base, device_id);
    info.vendor_id = vendor_id;
    info.command = pci_read(&info, struct pci_spec_device_info_base, command);
    info.status = pci_read(&info, struct pci_spec_device_info_base, status);
    info.revision_id =
        pci_read(&info, struct pci_spec_device_info_base, revision_id);

    info.prog_if = pci_read(&info, struct pci_spec_device_info_base, prog_if);
    info.header_kind = hdrkind;

    info.class = pci_read(&info, struct pci_spec_device_info_base, class_code);
    info.subclass = pci_read(&info, struct pci_spec_device_info_base, subclass);
    info.irq_pin = irq_pin;
    info.multifunction = is_multi_function;

    printk(LOGLEVEL_INFO,
           "\tdevice: " PCI_DEVICE_INFO_FMT "\n",
           PCI_DEVICE_INFO_FMT_ARGS(&info));

    const bool class_is_pci_bridge = info.class == 0x4 && info.subclass == 0x6;
    const bool hdrkind_is_pci_bridge =
        hdrkind == PCI_SPEC_DEVHDR_KIND_PCI_BRIDGE;

    if (hdrkind_is_pci_bridge != class_is_pci_bridge) {
        printk(LOGLEVEL_WARN,
               "pcie: invalid device, header-type and class/subclass "
               "mismatch\n");
        return;
    }

    // Disable I/O Space and Memory space flags to parse bars. Re-enable after.
    const uint16_t new_command =
        info.command &
        (uint16_t)~(__PCI_DEVCMDREG_IOSPACE | __PCI_DEVCMDREG_MEMSPACE);

    pci_write(&info, struct pci_spec_device_info_base, command, new_command);

    bool has_mmio_bar = false;
    bool has_io_bar = false;

    switch (hdrkind) {
        case PCI_SPEC_DEVHDR_KIND_GENERAL: {
            info.max_bar_count = PCI_BAR_COUNT_FOR_GENERAL;
            info.bar_list =
                kmalloc(sizeof(struct pci_device_bar_info) *
                        info.max_bar_count);

            if (info.bar_list == NULL) {
                printk(LOGLEVEL_WARN,
                       "pcie: failed to allocate memory for bar list\n");
                return;
            }

            pci_parse_capabilities(&info);
            for (uint8_t index = 0; index != info.max_bar_count; index++) {
                struct pci_device_bar_info *bar = &info.bar_list[index];
                const uint8_t bar_index = index;

                const enum parse_bar_result result =
                    pci_parse_bar(&info, &index, /*is_bridge=*/false, bar);

                if (result == E_PARSE_BAR_IGNORE) {
                    printk(LOGLEVEL_INFO,
                           "\t\tgeneral bar %" PRIu8 ": ignoring\n",
                           index);

                    bar->is_present = false;
                    continue;
                }

                if (result != E_PARSE_BAR_OK) {
                    printk(LOGLEVEL_WARN,
                           "pcie: failed to parse bar %" PRIu8 " for device\n",
                           index);

                    bar->is_present = false;
                    break;
                }

                if (bar->is_mmio) {
                    has_mmio_bar = true;
                } else {
                    has_io_bar = true;
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
                        bar->is_64_bit ? "64-bit, " : "32-bit, " : "",
                       bar->port_or_phys_range.size);

                bar++;
            }

            break;
        }
        case PCI_SPEC_DEVHDR_KIND_PCI_BRIDGE: {
            info.max_bar_count = PCI_BAR_COUNT_FOR_BRIDGE;
            info.bar_list =
                kmalloc(sizeof(struct pci_device_bar_info) *
                        info.max_bar_count);

            if (info.bar_list == NULL) {
                printk(LOGLEVEL_WARN,
                       "pcie: failed to allocate memory for bar list\n");
                return;
            }

            pci_parse_capabilities(&info);
            for (uint8_t index = 0; index != info.max_bar_count; index++) {
                struct pci_device_bar_info *bar = &info.bar_list[index];

                const uint8_t bar_index = index;
                const enum parse_bar_result result =
                    pci_parse_bar(&info, &index, /*is_bridge=*/true, bar);

                if (result == E_PARSE_BAR_IGNORE) {
                    printk(LOGLEVEL_INFO,
                           "\t\tbridge bar %" PRIu8 ": ignoring\n",
                           index);
                    continue;
                }

                if (result != E_PARSE_BAR_OK) {
                    printk(LOGLEVEL_INFO,
                           "pcie: failed to parse bar %" PRIu8 " for "
                           "device, " PCI_DEVICE_INFO_FMT "\n",
                           index,
                           PCI_DEVICE_INFO_FMT_ARGS(&info));
                    break;
                }

                if (bar->is_mmio) {
                    has_mmio_bar = true;
                } else {
                    has_io_bar = true;
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
                        bar->is_64_bit ? "64-bit, " : "32-bit, " : "",
                       bar->port_or_phys_range.size);

                bar++;
            }

            const uint8_t secondary_bus_number =
                pci_read(&info,
                         struct pci_spec_pci_to_pci_bridge_device_info,
                         secondary_bus_number);

            pci_parse_bus(domain, &info.config_space, secondary_bus_number);
            break;
        }
        case PCI_SPEC_DEVHDR_KIND_CARDBUS_BRIDGE:
            printk(LOGLEVEL_INFO,
                   "pcie: cardbus bridge not supported. ignoring");
            break;
    }

    // Enable writing to mmio and io space (when necessary).
    if (has_mmio_bar) {
        info.command |= __PCI_DEVCMDREG_MEMSPACE;
    }

    if (has_io_bar) {
        info.command |= __PCI_DEVCMDREG_IOSPACE;
    }

    pci_write(&info, struct pci_spec_device_info_base, command, info.command);

    struct pci_device_info *const info_out = kmalloc(sizeof(*info_out));
    if (info_out == NULL) {
        free_inside_device_info(&info);
        printk(LOGLEVEL_WARN,
               "pci: failed to allocate pci-device-info struct\n");

        return;
    }

    *info_out = info;

    list_add(&domain->device_list, &info_out->list_in_domain);
    list_add(&g_device_list, &info_out->list_in_devices);
}

void
pci_parse_bus(struct pci_domain *const domain,
              struct pci_config_space *const config_space,
              const uint8_t bus)
{
    config_space->bus = bus;
    for (uint8_t slot = 0; slot != PCI_MAX_DEVICE_COUNT; slot++) {
        config_space->device_slot = slot;
        for (uint8_t func = 0; func != PCI_MAX_FUNCTION_COUNT; func++) {
            config_space->function = func;
            parse_function(domain, config_space);
        }
    }
}

void pci_parse_domain(struct pci_domain *const domain) {
    struct pci_config_space config_space = {
        .domain_segment = domain->segment,
        .bus = 0,
        .device_slot = 0,
        .function = 0,
    };

    static uint64_t index = 0;
    printk(LOGLEVEL_INFO,
           "pci: domain #%" PRIu64 ": mmio at %p, first bus=%" PRIu64 ", "
           "end bus=%" PRIu64 ", segment: %" PRIu32 "\n",
           index + 1,
           domain->mmio->base,
           domain->bus_range.front,
           range_get_end_assert(domain->bus_range),
           domain->segment);

    index++;
    for (uint16_t i = 0; i != PCI_MAX_BUS_COUNT; i++) {
        pci_parse_bus(domain, &config_space, /*bus=*/i);
    }
}

struct pci_domain *
pci_add_pcie_domain(struct range bus_range,
                    const uint64_t base_addr,
                    const uint16_t segment)
{
    struct pci_domain *const domain = kmalloc(sizeof(*domain));
    if (domain == NULL) {
        return NULL;
    }

    list_init(&domain->list);
    list_init(&domain->device_list);

    const struct range config_space_range =
        RANGE_INIT(base_addr, align_up_assert(bus_range.size << 20, PAGE_SIZE));

    domain->mmio =
        vmap_mmio(config_space_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (domain->mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "pcie: failed to mmio-map pci-domain config-space\n");

        return NULL;
    }

    domain->bus_range = bus_range;
    domain->segment = segment;

    list_add(&g_domain_list, &domain->list);
    g_domain_count++;

    return domain;
}

void pci_init_drivers() {
    driver_foreach(driver) {
        if (driver->pci == NULL) {
            continue;
        }

        struct pci_driver *const pci_driver = driver->pci;
        struct pci_device_info *device = NULL;

        list_foreach(device, &g_device_list, list_in_devices) {
            if (pci_driver->match == PCI_DRIVER_MATCH_VENDOR) {
                if (device->vendor_id == pci_driver->vendor) {
                    pci_driver->init(device);
                }

                continue;
            }

            if (pci_driver->match == PCI_DRIVER_MATCH_VENDOR_DEVICE) {
                if (device->vendor_id != pci_driver->vendor) {
                    continue;
                }

                bool found = false;
                for (uint8_t i = 0; i != pci_driver->device_count; i++) {
                    if (pci_driver->devices[i] == device->id) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    pci_driver->init(device);
                }

                continue;
            }

            if (pci_driver->match & __PCI_DRIVER_MATCH_CLASS) {
                if (device->class != pci_driver->class) {
                    continue;
                }
            }

            if (pci_driver->match & __PCI_DRIVER_MATCH_SUBCLASS) {
                if (device->subclass != pci_driver->subclass) {
                    continue;
                }
            }

            if (pci_driver->match & __PCI_DRIVER_MATCH_PROGIF) {
                if (device->prog_if != pci_driver->prog_if) {
                    continue;
                }
            }

            pci_driver->init(device);
        }
    }
}

void pci_init() {
#if defined(__x86_64__)
    if (list_empty(&g_domain_list)) {
        struct pci_domain *const root_domain = kmalloc(sizeof(*root_domain));
        assert_msg(root_domain != NULL, "failed to allocate pci root domain");

        struct pci_device_info dev_0 = { .domain = root_domain };
        const uint32_t dev_0_first_dword =
            pci_read(&dev_0, struct pci_spec_device_info_base, vendor_id);

        if (dev_0_first_dword == PCI_READ_FAIL) {
            printk(LOGLEVEL_WARN,
                   "pci: failed to scan pci bus. aborting init\n");
            return;
        }

        list_init(&root_domain->list);
        list_init(&root_domain->device_list);

        pci_parse_domain(root_domain);
    } else {
#endif /* defined(__x86_64__) */
        struct pci_domain *domain = NULL;
        list_foreach(domain, &g_domain_list, list) {
            pci_parse_domain(domain);
        }
#if defined(__x86_64__)
    }
#endif /* defined(__x86_64__) */

    pci_init_drivers();
}
