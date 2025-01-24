/*
 * kernel/src/dev/pci/domain.c
 * © suhas pai
 */

#include "dev/pci/bus.h"
#include "dev/pci/device.h"
#include "dev/pci/ecam.h"

#if defined(__x86_64__)
    #include "dev/pci/legacy.h"
#endif /* defined(__x86_64__) */

#include "dev/device.h"

static bool pci_domain_probe(struct bus *const bus) {
    struct pci_domain *const domain = parent_of(bus, struct pci_domain, bus);
    pci_domain_foreach_bus(domain, pci_bus) {
        if (!bus_probe(&pci_bus->bus)) {
            return false;
        }
    }

    return true;
}

void
pci_domain_init(struct pci_domain *const domain,
                struct bus *const parent,
                const enum pci_domain_kind kind,
                const uint16_t segment)
{
    bus_init(&domain->bus,
             parent,
             /*name=*/SV_EMPTY(),
             &pci_device()->bus,
             pci_domain_probe);

    domain->kind = kind;
    domain->segment = segment;
}

struct pci_bus *pci_domain_get_root_bus(struct pci_domain *const domain) {
    return list_head(&domain->bus.device_list, struct pci_bus, entity_list);
}

__debug_optimize(3) uint8_t
pci_domain_read_8(const struct pci_domain *const domain,
                  const struct pci_location *const loc,
                  const uint16_t offset)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            return pci_legacy_domain_read(loc, offset, sizeof(uint8_t));
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            return pci_ecam_read_8((const struct pci_domain_ecam *)domain,
                                   loc,
                                   offset);
    }

    verify_not_reached();
}

__debug_optimize(3) uint16_t
pci_domain_read_16(const struct pci_domain *const domain,
                   const struct pci_location *const loc,
                   const uint16_t offset)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            return pci_legacy_domain_read(loc, offset, sizeof(uint16_t));
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            return pci_ecam_read_16((const struct pci_domain_ecam *)domain,
                                    loc,
                                    offset);
    }

    verify_not_reached();
}

__debug_optimize(3) uint32_t
pci_domain_read_32(const struct pci_domain *const domain,
                   const struct pci_location *const loc,
                   const uint16_t offset)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            return pci_legacy_domain_read(loc, offset, sizeof(uint32_t));
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            return pci_ecam_read_32((const struct pci_domain_ecam *)domain,
                                    loc,
                                    offset);
    }

    verify_not_reached();
}

__debug_optimize(3) uint64_t
pci_domain_read_64(const struct pci_domain *const domain,
                   const struct pci_location *const loc,
                   const uint16_t offset)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            return pci_legacy_domain_read(loc, offset, sizeof(uint64_t));
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            return pci_ecam_read_64((const struct pci_domain_ecam *)domain,
                                    loc,
                                    offset);
    }

    verify_not_reached();
}

__debug_optimize(3) void
pci_domain_write_8(const struct pci_domain *const domain,
                   const struct pci_location *const loc,
                   const uint16_t offset,
                   const uint8_t value)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            pci_legacy_domain_write(loc, offset, value, sizeof(uint8_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            pci_ecam_write_8((const struct pci_domain_ecam *)domain,
                             loc,
                             offset,
                             value);
            return;
    }

    verify_not_reached();
}

__debug_optimize(3) void
pci_domain_write_16(const struct pci_domain *const domain,
                    const struct pci_location *const loc,
                    const uint16_t offset,
                    const uint16_t value)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            pci_legacy_domain_write(loc, offset, value, sizeof(uint16_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            pci_ecam_write_16((const struct pci_domain_ecam *)domain,
                              loc,
                              offset,
                              value);
            return;
    }

    verify_not_reached();
}

__debug_optimize(3) void
pci_domain_write_32(const struct pci_domain *const domain,
                    const struct pci_location *const loc,
                    const uint16_t offset,
                    const uint32_t value)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            pci_legacy_domain_write(loc, offset, value, sizeof(uint32_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            pci_ecam_write_32((const struct pci_domain_ecam *)domain,
                              loc,
                              offset,
                              value);
            return;
    }

    verify_not_reached();
}

__debug_optimize(3) void
pci_domain_write_64(const struct pci_domain *const domain,
                    const struct pci_location *const loc,
                    const uint16_t offset,
                    const uint64_t value)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            verify_not_reached();
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            pci_ecam_write_64((const struct pci_domain_ecam *)domain,
                              loc,
                              offset,
                              value);
            return;
    }

    verify_not_reached();
}
