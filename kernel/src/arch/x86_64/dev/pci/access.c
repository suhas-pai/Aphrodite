/*
 * kernel/arch/x86_64/dev/pci/read.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/pci/structs.h"

#include "dev/port.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/util.h"

#include "sys/mmio.h"

__optimize(3) uint32_t
arch_pcie_read(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint8_t access_size)
{
    if (!index_range_in_bounds(RANGE_INIT(offset, access_size),
                               sizeof(struct pci_spec_device_info)))
    {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to read %" PRIu8 " bytes at offset %" PRIu32
               ", which falls outside pci-device's descriptor\n",
               access_size,
               offset);
        return PCI_READ_FAIL;
    }

    switch (access_size) {
        case sizeof(uint8_t):
            return mmio_read_8(device->pcie_info + offset);
        case sizeof(uint16_t):
            return mmio_read_16(device->pcie_info + offset);
        case sizeof(uint32_t):
            return mmio_read_32(device->pcie_info + offset);
    }

    verify_not_reached();
}

__optimize(3) bool
arch_pcie_write(const struct pci_device_info *const device,
                const uint32_t offset,
                const uint32_t value,
                const uint8_t access_size)
{
    if (!index_range_in_bounds(RANGE_INIT(offset, access_size),
                               sizeof(struct pci_spec_device_info)))
    {
        printk(LOGLEVEL_WARN,
               "pcie: attempting to write %" PRIu8 " bytes to offset %" PRIu32
               ", which falls outside pci-device's descriptor\n",
               access_size,
               offset);
        return false;
    }

    switch (access_size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            mmio_write_8(device->pcie_info + offset, value);

            return true;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            mmio_write_16(device->pcie_info + offset, value);

            return true;
        case sizeof(uint32_t):
            mmio_write_32(device->pcie_info + offset, value);
            return true;
    }

    verify_not_reached();
}

enum pci_config_address_flags {
    __PCI_CONFIG_ADDR_ENABLE = 1ull << 31,
};

__optimize(3) static inline void
seek_to_config_space(const struct pci_config_space *const config_space,
                     const uint32_t offset)
{
    const uint32_t address =
        align_down(offset, /*boundary=*/4) |
        ((uint32_t)config_space->function << 8) |
        ((uint32_t)config_space->device_slot << 11)  |
        ((uint32_t)config_space->bus << 16) |
        __PCI_CONFIG_ADDR_ENABLE;

    port_out32(PORT_PCI_CONFIG_ADDRESS, address);
}


__optimize(3) uint32_t
arch_pci_read(const struct pci_device_info *const device,
              const uint32_t offset,
              const uint8_t access_size)
{
    if (device->supports_pcie) {
        return arch_pcie_read(device, offset, access_size);
    }

    seek_to_config_space(&device->config_space, offset);
    switch (access_size) {
        case sizeof(uint8_t):
            return port_in8(PORT_PCI_CONFIG_DATA + (offset & 0b11));
        case sizeof(uint16_t):
            return port_in16(PORT_PCI_CONFIG_DATA + (offset & 0b11));
        case sizeof(uint32_t):
            return port_in32(PORT_PCI_CONFIG_DATA + (offset & 0b11));
    }

    verify_not_reached();
}

__optimize(3) bool
arch_pci_write(const struct pci_device_info *const device,
               const uint32_t offset,
               const uint32_t value,
               const uint8_t access_size)
{
    if (device->supports_pcie) {
        return arch_pcie_write(device, offset, value, access_size);
    }

    seek_to_config_space(&device->config_space, offset);
    switch (access_size) {
        case sizeof(uint8_t):
            assert(value <= UINT8_MAX);
            port_out8(PORT_PCI_CONFIG_DATA + (offset & 0b11), value);

            return true;
        case sizeof(uint16_t):
            assert(value <= UINT16_MAX);
            port_out16(PORT_PCI_CONFIG_DATA + (offset & 0b11), value);

            return true;
        case sizeof(uint32_t):
            port_out32(PORT_PCI_CONFIG_DATA + (offset & 0b11), value);
            return true;
    }

    return false;
}