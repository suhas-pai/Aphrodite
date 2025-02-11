/*
 * kernel/src/dev/pci/init.c
 * © suhas pai
 */

#include "dev/pci/device.h"
#include "dev/pci/driver.h"

#if defined(__x86_64__)
    #include "dev/pci/legacy.h"
#endif /* defined(__x86_64__) */

#include "dev/pci/entity.h"
#include "dev/pci/structs.h"

#include "dev/init.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"

void pci_init_drivers() {
    pci_device_foreach_driver(driver) {
        pci_device_foreach_entity(entity) {
            if (driver->match == PCI_DRIVER_MATCH_VENDOR) {
                if (entity->vendor_id == driver->vendor) {
                    entity->device.driver = &driver->driver;
                    device_probe(&entity->device);

                    break;
                }

                continue;
            }

            if (driver->match == PCI_DRIVER_MATCH_VENDOR_DEVICE) {
                if (entity->vendor_id != driver->vendor) {
                    continue;
                }

                bool found = false;
                ptrarr_foreach(driver->devices, driver->device_count, id) {
                    if (*id == entity->id) {
                        found = true;
                        break;
                    }
                }

                if (found) {
                    entity->device.driver = &driver->driver;
                    device_probe(&entity->device);

                    break;
                }

                continue;
            }

            if (driver->match & __PCI_DRIVER_MATCH_CLASS) {
                if (entity->class != driver->class) {
                    continue;
                }
            }

            if (driver->match & __PCI_DRIVER_MATCH_SUBCLASS) {
                if (entity->subclass != driver->subclass) {
                    continue;
                }
            }

            if (driver->match & __PCI_DRIVER_MATCH_PROGIF) {
                if (entity->prog_if != driver->prog_if) {
                    continue;
                }
            }

            entity->device.driver = &driver->driver;
            device_probe(&entity->device);

            break;
        }
    }
}

__debug_optimize(3) void pci_init() {
    const int flag = spin_acquire_save_intr(&pci_device()->bus.device.lock);
#if defined(__x86_64__)
    if (list_empty(&pci_device()->bus.device_list)) {
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
            spin_release_restore_intr(&pci_device()->bus.device.lock, flag);
            printk(LOGLEVEL_WARN,
                   "pci: failed to find pci bus in legacy domain. aborting "
                   "init\n");

            return;
        }

        struct pci_domain *const legacy_domain =
            kmalloc(sizeof(*legacy_domain));

        assert_msg(legacy_domain != nullptr,
                   "pci: failed to allocate pci-legacy root domain");

        pci_domain_init(legacy_domain,
                        dev_root_bus(),
                        PCI_DOMAIN_LEGACY,
                        /*segment=*/0);

        struct pci_bus *const root_bus =
            pci_bus_create(legacy_domain, /*bus_id=*/0, /*segment=*/0);

        assert_msg(root_bus != nullptr,
                   "pci: failed to allocate pci-legacy root bus");
    }
#else
    if (list_empty(&pci_device()->bus.device_list)) {
        spin_release_restore_intr(&pci_device()->bus.device.lock, flag);
        printk(LOGLEVEL_INFO, "pci: no root-bus found. Aborting init\n");

        return;
    }
#endif /* defined(__x86_64__) */

    pci_init_drivers();
}
