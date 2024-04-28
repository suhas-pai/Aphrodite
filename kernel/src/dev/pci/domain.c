/*
 * kernel/src/dev/pci/domain.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "dev/pci/legacy.h"
#endif /* defined(__x86_64__) */

#include "lib/adt/array.h"

#include "cpu/spinlock.h"
#include "ecam.h"

static struct array g_domain_list = ARRAY_INIT(sizeof(struct pci_domain *));
static struct spinlock g_domain_lock = SPINLOCK_INIT();

__optimize(3) bool pci_add_domain(struct pci_domain *const domain) {
    const int flag = spin_acquire_save_irq(&g_domain_lock);
    if (!array_append(&g_domain_list, &domain)) {
        spin_release_restore_irq(&g_domain_lock, flag);
        return false;
    }

    spin_release_restore_irq(&g_domain_lock, flag);
    return true;
}

__optimize(3) bool pci_remove_domain(struct pci_domain *const domain) {
    uint32_t index = 0;
    const int flag = spin_acquire_save_irq(&g_domain_lock);

    array_foreach(&g_domain_list, const struct pci_domain *, iter) {
        if (*iter == domain) {
            array_remove_index(&g_domain_list, index);
            spin_release_restore_irq(&g_domain_lock, flag);

            return true;
        }

        index++;
    }

    spin_release_restore_irq(&g_domain_lock, flag);
    return false;
}

__optimize(3)
const struct array *pci_get_domain_list_locked(int *const flag_out) {
    *flag_out = spin_acquire_save_irq(&g_domain_lock);
    return &g_domain_list;
}

__optimize(3) void pci_release_domain_list_lock(const int flag) {
    spin_release_restore_irq(&g_domain_lock, flag);
}

__optimize(3) uint8_t
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
            return pci_ecam_read_8((const struct pci_ecam_domain *)domain,
                                   loc,
                                   offset);
    }

    verify_not_reached();
}

__optimize(3) uint16_t
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
            return pci_ecam_read_16((const struct pci_ecam_domain *)domain,
                                    loc,
                                    offset);
    }

    verify_not_reached();
}

__optimize(3) uint32_t
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
            return pci_ecam_read_32((const struct pci_ecam_domain *)domain,
                                    loc,
                                    offset);
    }

    verify_not_reached();
}

__optimize(3) uint64_t
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
            return pci_ecam_read_64((const struct pci_ecam_domain *)domain,
                                    loc,
                                    offset);
    }

    verify_not_reached();
}

__optimize(3) void
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
            pci_ecam_write_8((const struct pci_ecam_domain *)domain,
                             loc,
                             offset,
                             value);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
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
            pci_ecam_write_16((const struct pci_ecam_domain *)domain,
                              loc,
                              offset,
                              value);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
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
            pci_ecam_write_32((const struct pci_ecam_domain *)domain,
                              loc,
                              offset,
                              value);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
pci_domain_write_64(const struct pci_domain *const domain,
                    const struct pci_location *const loc,
                    const uint16_t offset,
                    const uint64_t value)
{
    switch (domain->kind) {
    #if defined(__x86_64__)
        case PCI_DOMAIN_LEGACY:
            pci_legacy_domain_write(loc, offset, value, sizeof(uint64_t));
            return;
    #endif /* defined(__x86_64__) */

        case PCI_DOMAIN_ECAM:
            pci_ecam_write_64((const struct pci_ecam_domain *)domain,
                              loc,
                              offset,
                              value);
            return;
    }

    verify_not_reached();
}
