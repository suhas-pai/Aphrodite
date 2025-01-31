/*
 * kernel/src/arch/x86_64/dev/ps2/mouse.c
 * © suhas pai
 */

#include <lib/adt/array.h>
#include "dev/ps2/driver.h"

#include "acpi/bus.h"
#include "acpi/device.h"
#include "acpi/driver.h"

#include "dev/init.h"
#include "dev/printk.h"

bool ps2_mouse_probe(struct device *const the_device) {
    struct acpi_device *const device =
        parent_of(the_device, struct acpi_device, device);

    const auto resources = &device->resources;
    irq_number_t mouse_irq = 0;

    array_foreach(&resources->irq_list, const struct os_acpi_irq_info, irq) {
        printk(LOGLEVEL_INFO,
               "ps2/mouse: found irq:\n"
               "\t" "trigger: %s\n"
               "\t" "level: %s\n"
               "\t" "shared: %s\n"
               "\t" "wake capable: %s\n"
               "\t" "%" PRIu32 " irqs:\n",
               irq->trigger_mode == IRQ_TRIGGER_MODE_EDGE ? "edge" : "level",
               irq->polarity == IRQ_POLARITY_HIGH ? "high" : "low",
               irq->is_shared ? "yes" : "no",
               irq->wake_capable ? "yes" : "no",
               irq->irq_count);

        ptrarr_foreach(irq->irq_list, irq->irq_count, irq_num) {
            printk(LOGLEVEL_INFO, "\t\t" "irq %" PRIu32 "\n", *irq_num);

            struct irq_pin *const pin = isr_get_irq_pin(*irq_num);
            if (pin == nullptr) {
                printk(LOGLEVEL_WARN,
                       "ps2/mouse: irq " IRQ_NUMBER_FMT " not found\n",
                       *irq_num);

                continue;
            }

            irq_pin_setup(pin, irq->polarity, irq->trigger_mode);
            mouse_irq = *irq_num;
        }
    }

    ps2_init_mouse(mouse_irq);
    return true;
}

static const struct string_view pnp_ids[] = {
    SV_STATIC("PNP0F00"),
    SV_STATIC("PNP0F01"),
    SV_STATIC("PNP0F02"),
    SV_STATIC("PNP0F03"),
    SV_STATIC("PNP0F04"),
    SV_STATIC("PNP0F05"),
    SV_STATIC("PNP0F06"),
    SV_STATIC("PNP0F07"),
    SV_STATIC("PNP0F08"),
    SV_STATIC("PNP0F09"),
    SV_STATIC("PNP0F0A"),
    SV_STATIC("PNP0F0B"),
    SV_STATIC("PNP0F0C"),
    SV_STATIC("PNP0F0D"),
    SV_STATIC("PNP0F0E"),
    SV_STATIC("PNP0F0F"),
    SV_STATIC("PNP0F10"),
    SV_STATIC("PNP0F11"),
    SV_STATIC("PNP0F12"),
    SV_STATIC("PNP0F13"),
    SV_STATIC("PNP0F14"),
    SV_STATIC("PNP0F15"),
    SV_STATIC("PNP0F16"),
    SV_STATIC("PNP0F17"),
    SV_STATIC("PNP0F18"),
    SV_STATIC("PNP0F19"),
    SV_STATIC("PNP0F1A"),
    SV_STATIC("PNP0F1B"),
    SV_STATIC("PNP0F1C"),
    SV_STATIC("PNP0F1D"),
    SV_STATIC("PNP0F1E"),
    SV_STATIC("PNP0F1F"),
    SV_STATIC("PNP0F20"),
    SV_STATIC("PNP0F21"),
    SV_STATIC("PNP0F22"),
    SV_STATIC("PNP0F23"),
    SV_STATIC("PNP0FFC"),
    SV_STATIC("PNP0FFF"),
};

static uacpi_namespace_node *get_namespace() {
    return uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
}

static void init_mouse_driver() {
    static struct acpi_driver acpi_driver = {
        .pnp_ids = pnp_ids,
        .pnp_id_count = countof(pnp_ids),
        .resources_flags = OS_ACPI_DRIVER_RESOURCES_IRQ,
        .get_namespace = get_namespace,
    };

    driver_initialize(&acpi_driver.driver,
                      acpi_bus(),
                      SV_STATIC("ps2-mouse"),
                      ps2_mouse_probe,
                      /*remove=*/nullptr,
                      /*shutdown=*/nullptr,
                      /*suspend=*/nullptr,
                      /*resume=*/nullptr);
};

MAKE_DEV_INIT_FUNC(init_mouse_driver);
