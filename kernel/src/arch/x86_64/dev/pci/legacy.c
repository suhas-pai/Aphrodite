/*
 * kernel/src/arch/x86_64/dev/pci/legacy.c
 * © suhas pai
 */

#include "dev/pci/location.h"
#include "lib/align.h"

#include "../pio.h"

enum pci_config_address_flags {
    __PCI_CONFIG_ADDR_ENABLE = 1ull << 31,
};

__debug_optimize(3) static inline void
seek_to_space_location(const struct pci_location *const loc,
                       const uint32_t offset)
{
    const uint32_t address =
        align_down(offset, /*boundary=*/4)
        | (uint32_t)loc->function << 8
        | (uint32_t)loc->slot << 11
        | (uint32_t)loc->bus << 16
        | __PCI_CONFIG_ADDR_ENABLE;

    pio_write32(PIO_PORT_PCI_CONFIG_ADDRESS, address);
}


__debug_optimize(3) uint32_t
pci_legacy_domain_read(const struct pci_location *const loc,
                       const uint32_t offset,
                       const uint8_t access_size)
{
    seek_to_space_location(loc, offset);
    switch (access_size) {
        case sizeof(uint8_t):
            return pio_read8(PIO_PORT_PCI_CONFIG_DATA + (offset & 0b11));
        case sizeof(uint16_t):
            return pio_read16(PIO_PORT_PCI_CONFIG_DATA + (offset & 0b11));
        case sizeof(uint32_t):
            return pio_read32(PIO_PORT_PCI_CONFIG_DATA + (offset & 0b11));
    }

    verify_not_reached();
}

__debug_optimize(3) void
pci_legacy_domain_write(const struct pci_location *const loc,
                        const uint32_t offset,
                        const uint32_t value,
                        const uint8_t access_size)
{
    seek_to_space_location(loc, offset);
    switch (access_size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            pio_write8(PIO_PORT_PCI_CONFIG_DATA + (offset & 0b11), value);

            return;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            pio_write16(PIO_PORT_PCI_CONFIG_DATA + (offset & 0b11), value);

            return;
        case sizeof(uint32_t):
            pio_write32(PIO_PORT_PCI_CONFIG_DATA + (offset & 0b11), value);
            return;
    }

    verify_not_reached();
}