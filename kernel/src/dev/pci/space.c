/*
 * kernel/src/dev/pci/space.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "dev/pci/legacy.h"
#endif /* defined(__x86_64__) */

#include "lib/adt/array.h"
#include "cpu/spinlock.h"

#include "ecam.h"
#include "space.h"

static struct array g_space_list = ARRAY_INIT(sizeof(struct pci_space *));
static struct spinlock g_space_lock = SPINLOCK_INIT();

bool pci_add_space(struct pci_space *const space) {
    const int flag = spin_acquire_with_irq(&g_space_lock);
    if (!array_append(&g_space_list, &space)) {
        spin_release_with_irq(&g_space_lock, flag);
        return false;
    }

    spin_release_with_irq(&g_space_lock, flag);
    return true;
}

bool pci_remove_space(struct pci_space *const space) {
    if (!list_empty(&space->entity_list)) {
        return false;
    }

    uint32_t index = 0;
    const int flag = spin_acquire_with_irq(&g_space_lock);

    array_foreach(&g_space_list, struct pci_space *, iter) {
        if (*iter == space) {
            array_remove_index(&g_space_list, index);
            break;
        }

        index++;
    }

    spin_release_with_irq(&g_space_lock, flag);
    return true;
}

const struct array *pci_get_space_list_locked(int *const flag_out) {
    *flag_out = spin_acquire_with_irq(&g_space_lock);
    return &g_space_list;
}

void pci_release_space_list_lock(const int flag) {
    spin_release_with_irq(&g_space_lock, flag);
}

__optimize(3) uint8_t
pci_space_read_8(struct pci_space *const obj,
                 const struct pci_space_location *const loc,
                 const uint16_t offset)
{
    switch (obj->kind) {
    #if defined(__x86_64__)
        case PCI_SPACE_LEGACY:
            return pci_legacy_space_read(loc, offset, sizeof(uint8_t));
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            return pci_ecam_read_8((struct pci_ecam_space *)obj, loc, offset);
    }

    verify_not_reached();
}

__optimize(3) uint16_t
pci_space_read_16(struct pci_space *const obj,
                  const struct pci_space_location *const loc,
                  const uint16_t offset)
{
    switch (obj->kind) {
    #if defined(__x86_64__)
        case PCI_SPACE_LEGACY:
            return pci_legacy_space_read(loc, offset, sizeof(uint16_t));
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            return pci_ecam_read_16((struct pci_ecam_space *)obj, loc, offset);
    }

    verify_not_reached();
}

__optimize(3) uint32_t
pci_space_read_32(struct pci_space *const obj,
                  const struct pci_space_location *const loc,
                  const uint16_t offset)
{
    switch (obj->kind) {
    #if defined(__x86_64__)
        case PCI_SPACE_LEGACY:
            return pci_legacy_space_read(loc, offset, sizeof(uint32_t));
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            return pci_ecam_read_32((struct pci_ecam_space *)obj, loc, offset);
    }

    verify_not_reached();
}

__optimize(3) uint64_t
pci_space_read_64(struct pci_space *const obj,
                  const struct pci_space_location *const loc,
                  const uint16_t offset)
{
    switch (obj->kind) {
    #if defined(__x86_64__)
        case PCI_SPACE_LEGACY:
            return pci_legacy_space_read(loc, offset, sizeof(uint64_t));
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            return pci_ecam_read_64((struct pci_ecam_space *)obj, loc, offset);
    }

    verify_not_reached();
}

__optimize(3) void
pci_space_write_8(struct pci_space *const obj,
                  const struct pci_space_location *const loc,
                  const uint16_t offset,
                  const uint8_t value)
{
    switch (obj->kind) {
    case PCI_SPACE_LEGACY:
        #if defined(__x86_64__)
            pci_legacy_space_write(loc, offset, value, sizeof(uint8_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            pci_ecam_write_8((struct pci_ecam_space *)obj, loc, offset, value);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
pci_space_write_16(struct pci_space *const obj,
                   const struct pci_space_location *const loc,
                   const uint16_t offset,
                   const uint16_t value)
{
    switch (obj->kind) {
    case PCI_SPACE_LEGACY:
        #if defined(__x86_64__)
            pci_legacy_space_write(loc, offset, value, sizeof(uint16_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            pci_ecam_write_16((struct pci_ecam_space *)obj, loc, offset, value);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
pci_space_write_32(struct pci_space *const obj,
                   const struct pci_space_location *const loc,
                   const uint16_t offset,
                   const uint32_t value)
{
    switch (obj->kind) {
    #if defined(__x86_64__)
        case PCI_SPACE_LEGACY:
            pci_legacy_space_write(loc, offset, value, sizeof(uint32_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            pci_ecam_write_32((struct pci_ecam_space *)obj, loc, offset, value);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
pci_space_write_64(struct pci_space *const obj,
                   const struct pci_space_location *const loc,
                   const uint16_t offset,
                   const uint64_t value)
{
    switch (obj->kind) {
    #if defined(__x86_64__)
        case PCI_SPACE_LEGACY:
            pci_legacy_space_write(loc, offset, value, sizeof(uint64_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_SPACE_ECAM:
            pci_ecam_write_64((struct pci_ecam_space *)obj, loc, offset, value);
            return;
    }

    verify_not_reached();
}
