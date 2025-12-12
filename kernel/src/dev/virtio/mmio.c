/*
 * kernel/src/dev/virtio/mmio.c
 * © suhas pai
 */

#include "dev/dtb/bus.h"
#include "dev/dtb/device.h"
#include "dev/dtb/driver.h"

#include "dev/virtio/init.h"

#include "dev/init.h"
#include "dev/printk.h"

#include "sys/mmio.h"

static
struct virtio_device *virtio_mmio_init(struct virtio_device *const device) {
    // The driver MUST ignore a device with MagicValue which is not 0x74726976,
    // although it MAY report an error.

    volatile struct virtio_mmio_device *const device_hdr = device->mmio.header;
    if (mmio_read(&device_hdr->magic) != VIRTIO_MMIO_DEVICE_MAGIC) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: device's header has the wrong magic\n");
        return nullptr;
    }

    // The driver MUST ignore a device with Version which is not 0x2, although
    // it MAY report an error.

    const uint32_t version = mmio_read(&device_hdr->version);
    if (version != 1 && version != 2) {
        printk(LOGLEVEL_WARN, "virtio-mmio: device has the wrong version\n");
        return nullptr;
    }

    // The driver MUST ignore a device with DeviceID 0x0, but MUST NOT report
    // any error.

    if (mmio_read(&device_hdr->device_id) == 0) {
        return nullptr;
    }

    return virtio_device_init(device);
}

static bool virtio_mmio_dtb_probe(struct device *const the_device) {
    struct dtb_device *const device =
        parent_of(the_device, struct dtb_device, device);

    const struct devicetree_node *const node = device->node;
    const struct devicetree_prop_reg *const reg =
        cast_to_ptr(const struct devicetree_prop_reg,
                    devicetree_node_get_prop(node, DEVICETREE_PROP_REG));

    if (reg == nullptr) {
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
        array_front(reg->list, const struct devicetree_prop_reg_info);

    if (reg_info->size < sizeof(struct virtio_mmio_device)) {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: dtb-node's 'reg' property has a range smaller "
               "the size of the mmio-header struct\n");
        return false;
    }

    struct range mmio_range = RANGE_EMPTY();
    if (!range_create_and_verify(reg_info->address,
                                 reg_info->size,
                                 &mmio_range))
    {
        printk(LOGLEVEL_WARN,
               "virtio-mmio: dtb-node's 'reg' property's range overflows\n");
        return false;
    }

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

static void init_drivers() {
    static const struct string_view compat_list[] = {
        SV_STATIC("virtio,mmio")
    };

    static struct dtb_driver dtb_driver = {
        .match_flags = __DTB_DRIVER_MATCH_COMPAT,
        .compat_list = compat_list,
        .compat_count = countof(compat_list)
    };

    driver_initialize(&dtb_driver.driver,
                      &dtb_bus()->bus,
                      /*name=*/SV_STATIC("virtio-mmio"),
                      virtio_mmio_dtb_probe,
                      /*remove=*/nullptr,
                      /*shutdown=*/nullptr,
                      /*suspend=*/nullptr,
                      /*resume=*/nullptr);
}

MAKE_DEV_INIT_FUNC(init_drivers);
