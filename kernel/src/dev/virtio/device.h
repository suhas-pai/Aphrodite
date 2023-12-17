/*
 * kernel/src/dev/virtio/device.h
 * © suhas pai
 */

#pragma once
#include "lib/adt/array.h"

#include "mm/mmio.h"
#include "structs.h"

struct virtio_device_shmem_region {
    struct mmio_region *mmio;
    struct range phys_range;

    uint8_t id;
};

bool virtio_device_shmem_region_map(struct virtio_device_shmem_region *region);

void
virtio_device_shmem_region_unmap(struct virtio_device_shmem_region *region);

enum virtio_device_transport_kind {
    VIRTIO_DEVICE_TRANSPORT_MMIO,
    VIRTIO_DEVICE_TRANSPORT_PCI,
};

struct virtio_device {
    struct list list;
    union {
        struct {
            struct pci_entity_info *entity;
            volatile struct virtio_pci_common_cfg *common_cfg;

            struct range device_cfg;
            struct range notify_cfg_range;

            volatile void *notify_queue_select;

            uint32_t notify_off_multiplier;
            struct virtio_pci_cfg_cap *pci_cfg;
        } pci;
        struct {
            struct mmio_region *region;
            volatile struct virtio_mmio_device *header;
        } mmio;
    };

    // Array of struct virtio_device_shmem_region
    struct array shmem_regions;

    // Array of uint8_t
    struct array vendor_cfg_list;
    struct virtio_split_queue *queue_list;

    uint8_t queue_count;
    struct {
        uint8_t pci_cfg;
        uint8_t isr_cfg;
    } pci_offsets;

    enum virtio_device_transport_kind transport_kind : 1;
    enum virtio_device_kind kind : 6;
};

void
virtio_device_queue_select_and_notify(struct virtio_device *device,
                                      uint16_t queue_index);

#define VIRTIO_DEVICE_INIT(name, transport_kind_) \
    ((struct virtio_device){ \
        .list = LIST_INIT(name.list), \
        .pci.entity = NULL, \
        .pci.v1.common_cfg = NULL, \
        .pci.v1.device_cfg = RANGE_EMPTY(), \
        .pci.v1.notify_queue_select = NULL, \
        .pci.v1.pci_cfg = NULL, \
        .pci.trans.io_base = 0, \
        .mmio.region = NULL, \
        .mmio.header = NULL, \
        .shmem_regions = ARRAY_INIT(sizeof(struct virtio_device_shmem_region)),\
        .vendor_cfg_list = ARRAY_INIT(sizeof(uint8_t)), \
        .queue_list = NULL, \
        .queue_count = 0, \
        .pci_offsets.pci_cfg = 0, \
        .pci_offsets.isr_cfg = 0, \
        .transport_kind = (transport_kind_), \
        .kind = VIRTIO_DEVICE_KIND_INVALID \
    })
