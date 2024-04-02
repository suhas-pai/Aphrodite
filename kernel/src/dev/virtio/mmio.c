/*
 * kernel/src/dev/virtio/mmio.c
 * © suhas pai
 */

#include "dev/driver.h"
#include "dev/printk.h"

#include "sys/mmio.h"
#include "init.h"

static
struct virtio_device *virtio_mmio_init(struct virtio_device *const device) {
    // The driver MUST ignore a device with MagicValue which is not 0x74726976,
    // although it MAY report an error.

    volatile struct virtio_mmio_device *const device_hdr = device->mmio.header;
    if (mmio_read(&device_hdr->magic) != VIRTIO_MMIO_DEVICE_MAGIC) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: device's header has the wrong magic\n");
        return NULL;
    }

    // The driver MUST ignore a device with Version which is not 0x2, although
    // it MAY report an error.

    const uint32_t version = mmio_read(&device_hdr->version);
    if (version != 1 && version != 2) {
        printk(LOGLEVEL_WARN, "virtio-mmio: device has the wrong version\n");
        return NULL;
    }

    // The driver MUST ignore a device with DeviceID 0x0, but MUST NOT report
    // any error.

    if (mmio_read(&device_hdr->device_id) == 0) {
        return NULL;
    }

    return virtio_device_init(device);
}

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop_reg *const reg =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg == NULL) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: dtb-node is missing a 'reg' property\n");
        return false;
    }

    if (array_item_count(reg->list) != 1) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: dtb-node's 'reg' property is of the incorrect "
               "length\n");
        return false;
    }

    const struct devicetree_prop_reg_info *const reg_info =
        array_front(reg->list);

    if (reg_info->size < sizeof(struct virtio_mmio_device)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: dtb-node's 'reg' property has a range smaller "
               "the size of the mmio-header struct\n");
        return false;
    }

    struct range mmio_range = RANGE_INIT(reg_info->address, reg_info->size);
    if (!range_align_out(mmio_range, PAGE_SIZE, &mmio_range)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: range in dtb-node's 'reg' property is too "
               "large\n");
        return false;
    }

    struct mmio_region *const mmio =
        vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    struct virtio_device virt_device = VIRTIO_DEVICE_MMIO_INIT(virt_device);

    virt_device.mmio.region = mmio;
    virt_device.mmio.header =
        mmio->base + (reg_info->address - mmio_range.front);

    virtio_mmio_init(&virt_device);
    return true;
}

static const struct string_view compat_list[] = {
    SV_STATIC("virtio,mmio")
};

static const struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,
    .compat_list = compat_list,
    .compat_count = countof(compat_list)
};

__driver static const struct driver driver = {
    .name = SV_STATIC("virtio-driver"),
    .dtb = &dtb_driver,
    .pci = NULL
};