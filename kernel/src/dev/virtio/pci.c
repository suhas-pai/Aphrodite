/*
 * kernel/src/dev/virtio/pci.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/util.h"
#include "sys/mmio.h"

#include "driver.h"
#include "init.h"

static void init_from_pci(struct pci_entity_info *const pci_entity) {
    enum virtio_device_kind device_kind = pci_entity->id;
    const char *kind = NULL;

    switch ((enum virtio_pci_trans_device_kind)device_kind) {
        case VIRTIO_PCI_TRANS_DEVICE_KIND_NETWORK_CARD:
            kind = "network-card";
            device_kind = VIRTIO_DEVICE_KIND_NETWORK_CARD;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_BLOCK_DEVICE:
            kind = "block-device";
            device_kind = VIRTIO_DEVICE_KIND_BLOCK_DEVICE;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_MEM_BALLOON_TRAD:
            kind = "memory-ballooning-trad";
            device_kind = VIRTIO_DEVICE_KIND_MEM_BALLOON_TRAD;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_CONSOLE:
            kind = "console";
            device_kind = VIRTIO_DEVICE_KIND_CONSOLE;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_SCSI_HOST:
            kind = "scsi-host";
            device_kind = VIRTIO_DEVICE_KIND_SCSI_HOST;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_ENTROPY_SRC:
            kind = "entropy-source";
            device_kind = VIRTIO_DEVICE_KIND_ENTROPY_SRC;

            break;
        case VIRTIO_PCI_TRANS_DEVICE_KIND_9P_TRANSPORT:
            kind = "9p-transport";
            device_kind = VIRTIO_DEVICE_KIND_9P_TRANSPORT;

            break;
    }

    if (kind == NULL) {
        const struct range device_id_range = range_create_end(0x1040, 0x107f);
        if (!range_has_index(device_id_range, device_kind)) {
            printk(LOGLEVEL_WARN,
                   "virtio-pci: device-id (0x%" PRIx16 ") is invalid\n",
                   device_kind);
            return;
        }

        kind = virtio_device_kind_string[device_kind].begin;
    }

    const bool is_trans = pci_entity->revision_id == 0;
    printk(LOGLEVEL_INFO,
           "virtio-pci: recognized %s%s\n",
           kind,
           is_trans ? " (transitional)" : "");

    const uint8_t cap_count = array_item_count(pci_entity->vendor_cap_list);
    if (cap_count == 0) {
        printk(LOGLEVEL_WARN, "virtio-pci: device has no capabilities\n");
        return;
    }

    struct virtio_device virt_device;

    virt_device.kind = device_kind;
    virt_device.transport_kind = VIRTIO_DEVICE_TRANSPORT_PCI;
    virt_device.is_transitional = is_trans;
    virt_device.pci.entity = pci_entity;

    pci_entity_enable_privl(pci_entity,
                            __PCI_ENTITY_PRIVL_BUS_MASTER |
                            __PCI_ENTITY_PRIVL_MEM_ACCESS);

#define pci_read_virtio_cap_field(field) \
    pci_read_from_base(pci_entity, \
                       *iter, \
                       struct virtio_pci_cap64, \
                       field)

    struct range notify_cfg_range = RANGE_EMPTY();

    uint8_t cap_index = 0;
    uint32_t notify_off_multiplier = 0;

    array_foreach(&pci_entity->vendor_cap_list, uint8_t, iter) {
        const uint8_t cap_len = pci_read_virtio_cap_field(cap.cap_len);
        if (cap_len < sizeof(struct virtio_pci_cap)) {
            printk(LOGLEVEL_INFO,
                   "\tvirtio-pci: capability-length (%" PRIu8 ") is too "
                   "short\n",
                   cap_len);

            cap_index++;
            continue;
        }

        const uint8_t bar_index = pci_read_virtio_cap_field(cap.bar);
        if (!index_in_bounds(bar_index, pci_entity->max_bar_count)) {
            printk(LOGLEVEL_INFO,
                   "\tvirtio-pci: index of base-address-reg (%" PRIu8 ") bar "
                   "is past end of pci-device's bar list\n",
                   bar_index);

            cap_index++;
            continue;
        }

        struct pci_entity_bar_info *const bar =
            &pci_entity->bar_list[bar_index];

        if (!bar->is_present) {
            printk(LOGLEVEL_INFO,
                   "\tvirtio-pci: base-address-reg of bar %" PRIu8 " is not "
                   "present\n",
                   bar_index);

            cap_index++;
            continue;
        }

        if (bar->is_mmio) {
            if (!pci_map_bar(bar)) {
                printk(LOGLEVEL_INFO,
                       "\tvirtio-pci: failed to map bar at index %" PRIu8 "\n",
                       bar_index);

                cap_index++;
                continue;
            }
        }

        const uint8_t cfg_type = pci_read_virtio_cap_field(cap.cfg_type);

        uint64_t offset = pci_read_virtio_cap_field(cap.offset);
        uint64_t length = pci_read_virtio_cap_field(cap.length);

        const struct range index_range = RANGE_INIT(offset, length);
        const struct range io_range =
            bar->is_mmio ?
                mmio_region_get_range(bar->mmio) : bar->port_or_phys_range;

        if (!range_has_index_range(io_range, index_range)) {
            printk(LOGLEVEL_WARN,
                   "\tvirtio-pci: capability has an offset+length pair that "
                   "falls outside device's io-range (" RANGE_FMT ")\n",
                   RANGE_FMT_ARGS(io_range));

            cap_index++;
            continue;
        }

        const char *cfg_kind = NULL;
        switch (cfg_type) {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                if (length < sizeof(struct virtio_pci_common_cfg)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: common-cfg capability is too "
                           "short\n");

                    cap_index++;
                    continue;
                }

                virt_device.pci.common_cfg = bar->mmio->base + offset;
                cfg_kind = "common-cfg";

                break;
            case VIRTIO_PCI_CAP_NOTIFY_CFG: {
                if (cap_len < sizeof(struct virtio_pci_notify_cfg_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: notify-cfg capability is too "
                           "short\n");

                    cap_index++;
                    continue;
                }

                notify_off_multiplier =
                    pci_read_from_base(pci_entity,
                                       *iter,
                                       struct virtio_pci_notify_cfg_cap,
                                       notify_off_multiplier);

                notify_cfg_range =
                    RANGE_INIT((uint64_t)bar->mmio->base + offset, length);

                cfg_kind = "notify-cfg";
                virt_device.pci.notify_queue_select =
                    (void *)notify_cfg_range.front;

                break;
            }
            case VIRTIO_PCI_CAP_ISR_CFG:
                if (cap_len < sizeof(struct virtio_pci_isr_cfg_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: isr-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.pci_offsets.isr_cfg = *iter;
                cfg_kind = "isr-cfg";

                break;
            case VIRTIO_PCI_CAP_DEVICE_CFG:
                virt_device.pci.device_cfg =
                    RANGE_INIT((uint64_t)bar->mmio->base + offset, length);

                cfg_kind = "device-cfg";
                break;
            case VIRTIO_PCI_CAP_PCI_CFG:
                if (cap_len < sizeof(struct virtio_pci_cap)) {
                    printk(LOGLEVEL_WARN,
                           "\tvirtio-pci: pci-cfg capability is too short\n");

                    cap_index++;
                    continue;
                }

                virt_device.pci_offsets.pci_cfg = *iter;
                cfg_kind = "pci-cfg";

                break;
            case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG:
                offset |= (uint64_t)pci_read_virtio_cap_field(offset_hi) << 32;
                length |= (uint64_t)pci_read_virtio_cap_field(length_hi) << 32;

                const struct virtio_device_shmem_region region = {
                    .phys_range = RANGE_INIT(offset, length),
                    .id = pci_read_virtio_cap_field(cap.id),
                };

                if (!array_append(&virt_device.shmem_regions, &region)) {
                    printk(LOGLEVEL_WARN,
                           "virtio-pci: failed to add shmem region to "
                           "array\n");

                    cap_index++;
                    continue;
                }

                cfg_kind = "shared-memory-cfg";
                break;
            case VIRTIO_PCI_CAP_VENDOR_CFG:
                if (!array_append(&virt_device.vendor_cfg_list, iter)) {
                    printk(LOGLEVEL_WARN,
                           "virtio-pci: failed to add vendor-cfg to array\n");

                    cap_index++;
                    continue;
                }

                cfg_kind = "vendor-cfg";
                break;
        }

        const struct range interface_range =
            subrange_to_full(io_range, index_range);

        printk(LOGLEVEL_INFO,
               "\tcapability %" PRIu8 ": %s\n"
               "\t\toffset: 0x%" PRIx64 "\n"
               "\t\tlength: %" PRIu64 "\n"
               "\t\t%s: " RANGE_FMT "\n",
               cap_index,
               cfg_kind,
               offset,
               length,
               bar->is_mmio ? "mmio" : "ports",
               RANGE_FMT_ARGS(interface_range));

        cap_index++;
    }

    if (virt_device.pci.common_cfg == NULL) {
        printk(LOGLEVEL_WARN, "virtio-pci: device is missing a common-cfg\n");
        return;
    }

    if (virt_device.pci.notify_queue_select != NULL) {
        const uint16_t notify_offset =
            le_to_cpu(mmio_read(&virt_device.pci.common_cfg->queue_notify_off));

        uint16_t full_off = 0;
        if (__builtin_expect(
                !check_mul(notify_off_multiplier, notify_offset, &full_off), 0))
        {
            printk(LOGLEVEL_WARN,
                   "virtio-pci: notify-cfg has a multipler * offset "
                   "(%" PRIu32 " * %" PRIu16 ") is beyond end of uint16_t "
                   "space\n",
                   notify_off_multiplier,
                   notify_offset);
            return;
        }

        if (!range_has_index(notify_cfg_range, full_off)) {
            printk(LOGLEVEL_WARN,
                   "virtio-pci: notify-cfg has a multipler * offset "
                   "(%" PRIu32 " * %" PRIu16 " = %" PRIu16 ") is beyond end of "
                   "notify-cfg's config space\n",
                   notify_off_multiplier,
                   full_off,
                   notify_offset);
            return;
        }

        virt_device.pci.notify_queue_select += full_off;
    }

#undef pci_read_virtio_cap_field
    virtio_device_init(&virt_device);
}

static const struct pci_driver pci_driver = {
    .init = init_from_pci,
    .match = PCI_DRIVER_MATCH_VENDOR,
    .vendor = 0x1af4,
};

__driver static const struct driver driver = {
    .name = "virtio-driver",
    .dtb = NULL,
    .pci = &pci_driver
};