/*
 * kernel/src/dev/pci/domain.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>

#include "lib/adt/array.h"
#include "location.h"

enum pci_domain_kind {
#if defined(__x86_64__)
    PCI_DOMAIN_LEGACY,
#endif /* defined(__x86_64__) */
    PCI_DOMAIN_ECAM
};

struct pci_domain {
    enum pci_domain_kind kind;
    uint16_t segment;
    uint64_t padding;
};

#define PCI_SPACE_MAX_OFFSET 0x1000

bool pci_add_domain(struct pci_domain *domain);
bool pci_remove_domain(struct pci_domain *domain);

const struct array *pci_get_domain_list_locked(int *flag_out);
void pci_release_domain_list_lock(int flag);

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
        __auto_type __result__ = (typeof_field(type, field))0; \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                __result__ = \
                    pci_domain_read_8((entity)->bus->domain, \
                                      &(entity)->loc, \
                                      offsetof(type, field)); \
                break; \
            case sizeof(uint16_t): \
                __result__ = \
                    pci_domain_read_16((entity)->bus->domain, \
                                       &(entity)->loc, \
                                       offsetof(type, field)); \
                break; \
            case sizeof(uint32_t): \
                __result__ = \
                    pci_domain_read_32((entity)->bus->domain, \
                                       &(entity)->loc, \
                                       offsetof(type, field)); \
                break; \
            case sizeof(uint64_t): \
                __result__ = \
                    pci_domain_read_64((entity)->bus->domain, \
                                       &(entity)->loc, \
                                       offsetof(type, field)); \
                break; \
        } \
        __result__; \
    })

#define pci_write(entity, type, field, value) \
    ({ \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                pci_domain_write_8((entity)->bus->domain, \
                                   &(entity)->loc, \
                                   offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint16_t): \
                pci_domain_write_16((entity)->bus->domain, \
                                    &(entity)->loc, \
                                    offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint32_t): \
                pci_domain_write_32((entity)->bus->domain, \
                                    &(entity)->loc, \
                                    offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint64_t): \
                pci_domain_write_64((entity)->bus->domain, \
                                    &(entity)->loc, \
                                    offsetof(type, field), \
                                    value); \
                break; \
        } \
    })

#define pci_read_from_base(entity, base, type, field) \
    ({ \
        __auto_type __result__ = (typeof_field(type, field))0; \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                __result__ = \
                    pci_domain_read_8((entity)->bus->domain, \
                                      &(entity)->loc, \
                                      (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint16_t): \
                __result__ = \
                    pci_domain_read_16((entity)->bus->domain, \
                                       &(entity)->loc, \
                                       (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint32_t): \
                __result__ = \
                    pci_domain_read_32((entity)->bus->domain, \
                                       &(entity)->loc, \
                                       (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint64_t): \
                __result__ = \
                    pci_domain_read_64((entity)->bus->domain, \
                                       &(entity)->loc, \
                                       (base) + offsetof(type, field)); \
                break; \
        } \
        __result__; \
    })

#define pci_write_from_base(entity, base, type, field, value) \
    ({ \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                pci_domain_write_8((entity)->bus->domain, \
                                   &(entity)->loc, \
                                   (base) + offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint16_t): \
                pci_domain_write_16((entity)->bus->domain, \
                                    &(entity)->loc, \
                                    (base) + offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint32_t): \
                pci_domain_write_32((entity)->bus->domain, \
                                    &(entity)->loc, \
                                    (base) + offsetof(type, field), \
                                    value); \
                break; \
            case sizeof(uint64_t): \
                pci_domain_write_64((entity)->bus->domain, \
                                    &(entity)->loc, \
                                    (base) + offsetof(type, field), \
                                    value); \
                break; \
        } \
    })
