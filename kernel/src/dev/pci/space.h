/*
 * kernel/src/dev/pci/space.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/list.h"

enum pci_space_kind {
#if defined(__x86_64__)
    PCI_SPACE_LEGACY,
#endif /* defined(__x86_64__) */
    PCI_SPACE_ECAM
};

struct pci_space {
    enum pci_space_kind kind;
    struct list entity_list;

    uint16_t segment;
};

struct pci_space_location {
    uint32_t segment;
    uint32_t bus;
    uint32_t slot;
    uint32_t function;
};

#define PCI_SPACE_MAX_OFFSET 0x1000

bool pci_add_space(struct pci_space *space);
bool pci_remove_space(struct pci_space *space);

const struct array *pci_get_space_list_locked(int *flag_out);
void pci_release_space_list_lock(int flag);

uint8_t
pci_space_read_8(struct pci_space *space,
                 const struct pci_space_location *loc,
                 uint16_t offset);

uint16_t
pci_space_read_16(struct pci_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset);

uint32_t
pci_space_read_32(struct pci_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset);

uint64_t
pci_space_read_64(struct pci_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset);

void
pci_space_write_8(struct pci_space *space,
                  const struct pci_space_location *loc,
                  uint16_t offset,
                  uint8_t value);

void
pci_space_write_16(struct pci_space *space,
                   const struct pci_space_location *loc,
                   uint16_t offset,
                   uint16_t value);

void
pci_space_write_32(struct pci_space *space,
                   const struct pci_space_location *loc,
                   uint16_t offset,
                   uint32_t value);

void
pci_space_write_64(struct pci_space *space,
                   const struct pci_space_location *loc,
                   uint16_t offset,
                   uint64_t value);

#define pci_read(device, type, field) \
    ({ \
        __auto_type __result__ = (typeof_field(type, field))0; \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                __result__ = \
                    pci_space_read_8((device)->space, \
                                     &(device)->loc, \
                                     offsetof(type, field)); \
                break; \
            case sizeof(uint16_t): \
                __result__ = \
                    pci_space_read_16((device)->space, \
                                      &(device)->loc, \
                                      offsetof(type, field)); \
                break; \
            case sizeof(uint32_t): \
                __result__ = \
                    pci_space_read_32((device)->space, \
                                      &(device)->loc, \
                                      offsetof(type, field)); \
                break; \
            case sizeof(uint64_t): \
                __result__ = \
                    pci_space_read_64((device)->space, \
                                      &(device)->loc, \
                                      offsetof(type, field)); \
                break; \
        } \
        __result__; \
    })

#define pci_write(device, type, field, value) \
    ({ \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                pci_space_write_8((device)->space, \
                                   &(device)->loc, \
                                   offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint16_t): \
                pci_space_write_16((device)->space, \
                                   &(device)->loc, \
                                   offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint32_t): \
                pci_space_write_32((device)->space, \
                                   &(device)->loc, \
                                   offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint64_t): \
                pci_space_write_64((device)->space, \
                                   &(device)->loc, \
                                   offsetof(type, field), \
                                   value); \
                break; \
        } \
    })

#define pci_read_from_base(device, base, type, field) \
    ({ \
        __auto_type __result__ = (typeof_field(type, field))0; \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                __result__ = \
                    pci_space_read_8((device)->space, \
                                     &(device)->loc, \
                                     (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint16_t): \
                __result__ = \
                    pci_space_read_16((device)->space, \
                                      &(device)->loc, \
                                      (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint32_t): \
                __result__ = \
                    pci_space_read_32((device)->space, \
                                      &(device)->loc, \
                                      (base) + offsetof(type, field)); \
                break; \
            case sizeof(uint64_t): \
                __result__ = \
                    pci_space_read_64((device)->space, \
                                      &(device)->loc, \
                                      (base) + offsetof(type, field)); \
                break; \
        } \
        __result__; \
    })

#define pci_write_from_base(device, base, type, field, value) \
    ({ \
        switch (sizeof_field(type, field)) { \
            case sizeof(uint8_t): \
                pci_space_write_8((device)->space, \
                                   &(device)->loc, \
                                   (base) + offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint16_t): \
                pci_space_write_16((device)->space, \
                                   &(device)->loc, \
                                   (base) + offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint32_t): \
                pci_space_write_32((device)->space, \
                                   &(device)->loc, \
                                   (base) + offsetof(type, field), \
                                   value); \
                break; \
            case sizeof(uint64_t): \
                pci_space_write_64((device)->space, \
                                   &(device)->loc, \
                                   (base) + offsetof(type, field), \
                                   value); \
                break; \
        } \
    })
