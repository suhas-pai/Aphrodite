/*
 * kernel/dev/virtio/device.h
 * © suhas pai
 */

#pragma once

#include "lib/adt/array.h"
#include "mm/mmio.h"

#include "structs.h"
#include "transport.h"

struct virtio_device_shmem_region {
    union {
        struct mmio_region *mmio;
        struct range phys_range;
    };

    uint8_t id;
    bool mapped : 1;
};

bool virtio_device_shmem_region_map(struct virtio_device_shmem_region *region);

struct virtio_device {
    struct list list;
    union {
        struct {
            struct pci_device_info *pci_device;
            volatile struct virtio_pci_common_cfg *common_cfg;

            struct range device_cfg;
            volatile void *notify_queue_select;

            struct virtio_pci_cfg_cap *pci_cfg;
        } pci;
    };

    struct virtio_transport_ops ops;

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

    bool is_transitional : 1;
    enum virtio_device_kind kind;
};

#define VIRTIO_DEVICE_INIT(name) \
    ((struct virtio_device){ \
        .list = LIST_INIT(name.list), \
        .pci.pci_device = NULL, \
        .pci.common_cfg = NULL, \
        .pci.device_cfg = RANGE_EMPTY(), \
        .pci.notify_queue_select = NULL, \
        .pci.pci_cfg = NULL, \
        .ops = VIRTIO_TRANSPORT_OPS_INIT(), \
        .shmem_regions = ARRAY_INIT(sizeof(struct virtio_device_shmem_region)),\
        .vendor_cfg_list = ARRAY_INIT(sizeof(uint8_t)), \
        .queue_list = NULL, \
        .queue_count = 0, \
        .pci_offsets.pci_cfg = 0, \
        .pci_offsets.isr_cfg = 0, \
        .is_transitional = false, \
        .kind = VIRTIO_DEVICE_KIND_INVALID \
    })
