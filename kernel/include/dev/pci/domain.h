/*
 * kernel/include/dev/pci/domain.h
 * © suhas pai
 */

#pragma once

#include "dev/bus.h"
#include "location.h"

enum pci_domain_kind : uint8_t {
#if defined(__x86_64__)
    PCI_DOMAIN_LEGACY,
#endif /* defined(__x86_64__) */
    PCI_DOMAIN_ECAM
};

struct pci_domain {
    struct bus bus;

    uint16_t segment;
    enum pci_domain_kind kind;
};

#define pci_domain_foreach_bus(domain, iter) \
    bus_foreach_device(&domain->bus, struct pci_bus, bus.device.list, iter)

void
pci_domain_init(struct pci_domain *domain,
                struct bus *parent,
                enum pci_domain_kind kind,
                uint16_t segment);

#define PCI_SPACE_MAX_OFFSET 0x1000

struct pci_bus *pci_domain_get_root_bus(struct pci_domain *domain);

uint8_t
pci_domain_read_8(const struct pci_domain *domain,
                  const struct pci_location *loc,
                  uint16_t offset);

uint16_t
pci_domain_read_16(const struct pci_domain *domain,
                   const struct pci_location *loc,
                   uint16_t offset);

uint32_t
pci_domain_read_32(const struct pci_domain *domain,
                   const struct pci_location *loc,
                   uint16_t offset);

uint64_t
pci_domain_read_64(const struct pci_domain *domain,
                   const struct pci_location *loc,
                   uint16_t offset);

void
pci_domain_write_8(const struct pci_domain *domain,
                   const struct pci_location *loc,
                   uint16_t offset,
                   uint8_t value);

void
pci_domain_write_16(const struct pci_domain *domain,
                    const struct pci_location *loc,
                    uint16_t offset,
                    uint16_t value);

void
pci_domain_write_32(const struct pci_domain *domain,
                    const struct pci_location *loc,
                    uint16_t offset,
                    uint32_t value);

void
pci_domain_write_64(const struct pci_domain *domain,
                    const struct pci_location *loc,
                    uint16_t offset,
                    uint64_t value);

#define pci_read(entity, type, field) \
    ({ \
        auto h_var(result) = (typeof_field(type, field))0; \
        auto h_var(bus) = pci_entity_get_bus(entity); \
        \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                h_var(result) = \
                    pci_domain_read_8(pci_bus_get_domain(h_var(bus)), \
                                      &(entity)->loc, \
                                      offsetof(type, field)); \
                break; \
            case sizeof(uint16_t): \
                h_var(result) = \
                    pci_domain_read_16(pci_bus_get_domain(h_var(bus)), \
                                       &(entity)->loc, \
                                       offsetof(type, field)); \
                break; \
            case sizeof(uint32_t): \
                h_var(result) = \
                    pci_domain_read_32(pci_bus_get_domain(h_var(bus)), \
                                       &(entity)->loc, \
                                       offsetof(type, field)); \
                break; \
            case sizeof(uint64_t): \
                h_var(result) = \
                    pci_domain_read_64(pci_bus_get_domain(h_var(bus)), \
                                       &(entity)->loc, \
                                       offsetof(type, field)); \
                break; \
            default: \
                verify_not_reached(); \
        } \
        h_var(result); \
    })

#define pci_write(entity, type, field, value) \
    ({ \
        auto h_var(bus) = pci_entity_get_bus(entity); \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                pci_domain_write_8(pci_bus_get_domain(h_var(bus)), \
                                   &(entity)->loc, \
                                   offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint16_t): \
                pci_domain_write_16(pci_bus_get_domain(h_var(bus)), \
                                    &(entity)->loc, \
                                    offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint32_t): \
                pci_domain_write_32(pci_bus_get_domain(h_var(bus)), \
                                    &(entity)->loc, \
                                    offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint64_t): \
                pci_domain_write_64(pci_bus_get_domain(h_var(bus)), \
                                    &(entity)->loc, \
                                    offsetof(type, field), \
                                    value); \
                break; \
            default: \
                verify_not_reached(); \
        } \
    })

#define pci_read_from_base(entity, base, type, field) \
    ({ \
        auto h_var(result) = (typeof_field(type, field))0; \
        auto h_var(bus) = pci_entity_get_bus((entity)); \
        \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                h_var(result) = \
                    pci_domain_read_8(pci_bus_get_domain(h_var(bus)), \
                                      &(entity)->loc, \
                                      (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint16_t): \
                h_var(result) = \
                    pci_domain_read_16(pci_bus_get_domain(h_var(bus)), \
                                       &(entity)->loc, \
                                       (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint32_t): \
                h_var(result) = \
                    pci_domain_read_32(pci_bus_get_domain(h_var(bus)), \
                                       &(entity)->loc, \
                                       (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint64_t): \
                h_var(result) = \
                    pci_domain_read_64(pci_bus_get_domain(h_var(bus)), \
                                       &(entity)->loc, \
                                       (base) + offsetof(type, field)); \
                break; \
            default: \
                verify_not_reached(); \
        } \
        h_var(result); \
    })

#define pci_write_from_base(entity, base, type, field, value) \
    ({ \
        auto h_var(bus) = pci_entity_get_bus((entity)); \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                pci_domain_write_8(pci_bus_get_domain(h_var(bus)), \
                                   &(entity)->loc, \
                                   (base) + offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint16_t): \
                pci_domain_write_16(pci_bus_get_domain(h_var(bus)), \
                                    &(entity)->loc, \
                                    (base) + offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint32_t): \
                pci_domain_write_32(pci_bus_get_domain(h_var(bus)), \
                                    &(entity)->loc, \
                                    (base) + offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint64_t): \
                pci_domain_write_64(pci_bus_get_domain(h_var(bus)), \
                                    &(entity)->loc, \
                                    (base) + offsetof(type, field), \
                                    value); \
                break; \
            default: \
                verify_not_reached(); \
        } \
    })
