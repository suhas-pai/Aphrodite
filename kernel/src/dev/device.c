/*
 * kernel/src/dev/device.c
 * © suhas pai
 */

#include "pci/entity.h"
#include "device.h"

uint64_t device_get_id(struct device *const device) {
    switch (device->kind) {
        case DEVICE_KIND_PCI_ENTITY: {
            struct pci_entity_info *const entity =
                container_of(device, struct pci_entity_info, device);

            return pci_entity_get_requester_id(entity);
        }
    }

    verify_not_reached();
}
