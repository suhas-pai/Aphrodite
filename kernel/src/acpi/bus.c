/*
 * kernel/src/acpi/bus.h
 * © suhas pai
 */

#include <uacpi/resources.h>
#include <uacpi/utilities.h>

#include "acpi/bus.h"

#ifdef CONFIG_UACPI
    #include "acpi/device.h"
    #include "acpi/driver.h"
    #include "acpi/resources.h"
#endif /* CONFIG_UACPI */

#include "dev/bus.h"
#include "dev/init.h"

#ifdef CONFIG_UACPI
    #include "dev/printk.h"
    #include "mm/simple_alloc.h"
#endif /* CONFIG_UACPI */

struct acpi_bus {
    struct bus bus;
};

#ifdef CONFIG_UACPI
static struct simple_alloc g_alloc;

static inline struct acpi_driver *
match_driver_by_hid(const struct uacpi_id_string string) {
    bus_foreach_driver(acpi_bus(), struct acpi_driver, driver.list, drv) {
        arrptr_foreach(drv->pnp_ids, drv->pnp_id_count, id) {
            const uint32_t length = string.size - 1;
            if (sv_equals(*id, sv_create_nocheck(string.value, length))) {
                return drv;
            }
        }
    }

    return nullptr;
}

static inline struct acpi_driver *
match_driver_by_cid(const struct uacpi_pnp_id_list *const list) {
    bus_foreach_driver(acpi_bus(), struct acpi_driver, driver.list, drv) {
        arrptr_foreach(list->ids, list->num_ids, id) {
            const struct string_view sv =
                sv_create_nocheck(id->value, id->size - 1);

            arrptr_foreach(drv->pnp_ids, drv->pnp_id_count, drv_id) {
                if (sv_equals(*drv_id, sv)) {
                    return drv;
                }
            }
        }
    }

    return nullptr;
}

static struct simple_alloc g_alloc;

static uacpi_iteration_decision
acpi_init_one_device(void *const ctx,
                     uacpi_namespace_node *const node,
                     uacpi_u32 node_depth)
{
    (void)ctx;
    (void)node_depth;

    uacpi_namespace_node_info *info = nullptr;
    const uacpi_status ret = uacpi_get_namespace_node_info(node, &info);

    if (uacpi_unlikely_error(ret)) {
        const char *const path =
            uacpi_namespace_node_generate_absolute_path(node);

        printk(LOGLEVEL_WARN,
               "Unable to retrieve node %s, information: %s\n",
               path,
               uacpi_status_to_string(ret));

        uacpi_free_absolute_path(path);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    struct acpi_driver *drv = nullptr;
    if (info->flags & UACPI_NS_NODE_INFO_HAS_HID) {
        // Match the HID against every existing acpi_driver pnp id list
        drv = match_driver_by_hid(info->hid);
    }

    if (drv == nullptr && (info->flags & UACPI_NS_NODE_INFO_HAS_CID)) {
        // Match the CID list against every existing acpi_driver pnp id list
        drv = match_driver_by_cid(&info->cid);
    }

    if (drv == nullptr) {
        uacpi_free_namespace_node_info(info);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    struct acpi_device *const device = simple_alloc(&g_alloc, sizeof(*device));
    if (device == nullptr) {
        uacpi_free_namespace_node_info(info);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    device->node = node;
    device->resources = OS_ACPI_DEVICE_RESOURCES_INIT();

    const bool result =
        os_acpi_device_resources_collect(&device->resources,
                                         &g_alloc,
                                         node,
                                         drv->resources_flags);

    uacpi_free_namespace_node_info(info);
    if (!result) {
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    // FIXME: Make a name.
    device_initialize(&device->device,
                      acpi_bus(),
                      &drv->driver,
                      drv->driver.name);

    device_probe(&device->device);
    return UACPI_ITERATION_DECISION_CONTINUE;
}
#endif

static bool acpi_bus_probe(struct bus *const the_bus) {
    (void)the_bus;

#ifdef CONFIG_UACPI
    simple_alloc_init(&g_alloc);
    uacpi_namespace_for_each_child(uacpi_namespace_root(),
                                   acpi_init_one_device,
                                   /*ascending_callback=*/nullptr,
                                   UACPI_OBJECT_DEVICE_BIT,
                                   UACPI_MAX_DEPTH_ANY,
                                   /*user=*/nullptr);

#endif /* defined(CONFIG_UACPI) */

    return true;
}

static struct acpi_bus g_acpi_bus = {
    .bus = BUS_INIT(g_acpi_bus.bus, /*parent=*/nullptr, acpi_bus_probe),
};

__debug_optimize(3) struct bus *acpi_bus() {
    return &g_acpi_bus.bus;
}

static void acpi_bus_init() {
    bus_init_root(&g_acpi_bus.bus);
    bus_register(acpi_bus());
}

MAKE_DEV_INIT_FUNC(acpi_bus_init);
