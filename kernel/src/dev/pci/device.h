/*
 * kernel/dev/pci/device.h
 * © suhas pai
 */

#pragma once

#if defined(__x86_64__)
    #include "arch/x86_64/cpu.h"
#endif /* defined(__x86_64__) */

#include "lib/adt/array.h"
#include "lib/adt/bitmap.h"

#include "cpu/isr.h"

struct pci_config_space {
    uint16_t domain_segment; // PCIe only. 0 for PCI Local bus

    uint8_t bus;
    uint8_t device_slot;
    uint8_t function;
};

uint32_t pci_config_space_get_index(struct pci_config_space config_space);

enum pci_device_msi_support {
    PCI_DEVICE_MSI_SUPPORT_NONE,
    PCI_DEVICE_MSI_SUPPORT_MSI,
    PCI_DEVICE_MSI_SUPPORT_MSIX,
};

struct pci_device_bar_info {
    struct range port_or_phys_range;

    struct mmio_region *mmio;
    uint32_t index_in_mmio;

    bool is_present : 1;
    bool is_mmio : 1;
    bool is_prefetchable : 1;
    bool is_64_bit : 1;
    bool is_mapped : 1;
};

bool pci_map_bar(struct pci_device_bar_info *bar);

uint8_t
pci_device_bar_read8(struct pci_device_bar_info *const bar, uint32_t offset);

uint16_t
pci_device_bar_read16(struct pci_device_bar_info *const bar, uint32_t offset);

uint32_t
pci_device_bar_read32(struct pci_device_bar_info *const bar, uint32_t offset);

uint64_t
pci_device_bar_read64(struct pci_device_bar_info *const bar, uint32_t offset);

struct pci_device_info;
struct pci_domain {
    struct list list;
    struct list device_list;

    struct range bus_range;
    struct mmio_region *mmio;

    uint16_t segment;

    bool
    (*write)(const struct pci_device_info *space,
             uint32_t offset,
             uint32_t value,
             uint8_t access_size);
};

extern uint32_t
arch_pci_read(const struct pci_device_info *s,
              uint32_t offset,
              uint8_t access_size);

extern bool
arch_pci_write(const struct pci_device_info *s,
               uint32_t offset,
               uint32_t value,
               uint8_t access_size);

extern uint32_t
arch_pcie_read(const struct pci_device_info *s,
               uint32_t offset,
               uint8_t access_size);

extern bool
arch_pcie_write(const struct pci_device_info *s,
                uint32_t offset,
                uint32_t value,
                uint8_t access_size);

#define PCI_DEVICE_MAX_BAR_COUNT 6

struct pci_device_info {
    struct list list_in_domain;
    struct list list_in_devices;

    struct pci_domain *domain;
    struct pci_config_space config_space;
    volatile void *pcie_info;

    uint16_t id;
    uint16_t vendor_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;

    // prog_if = "Programming Interface"
    uint8_t prog_if;
    uint8_t header_kind;

    uint8_t subclass;
    uint8_t class;
    uint8_t irq_pin;

    bool supports_pcie : 1;
    bool multifunction : 1;
    uint8_t max_bar_count : 3;

    enum pci_device_msi_support msi_support : 2;

    union {
        uint64_t pcie_msi_offset;
        struct {
            uint64_t pcie_msix_offset;
            struct bitmap msix_table;
        };
    };

    // Array of uint8_t
    struct array vendor_cap_list;
    struct pci_device_bar_info *bar_list;
};

uint32_t pci_device_get_index(const struct pci_device_info *const device);

#define pci_read(device, type, field) \
    arch_pci_read((device), offsetof(type, field), sizeof_field(type, field))
#define pci_write(device, type, field, value) \
    arch_pci_write((device),\
                   offsetof(type, field),\
                   value, \
                   sizeof_field(type, field))

#define pci_read_with_offset(device, offset, type, field) \
    arch_pci_read((device), \
                  (offset) + offsetof(type, field), \
                  sizeof_field(type, field))

#define pci_write_with_offset(device, offset, type, field, value) \
    arch_pci_write((device),\
                   (offset) + offsetof(type, field), \
                   value,\
                   sizeof_field(type, field))

#define PCI_DEVICE_INFO_FMT                                                    \
    "%" PRIx8 ":%" PRIx8 ":%" PRIx8 ":%" PRIx16 ":%" PRIx16 " (%" PRIx8 ":%"   \
    PRIx8 ")"

#define PCI_DEVICE_INFO_FMT_ARGS(device)                                       \
    (device)->config_space.bus,                                                \
    (device)->config_space.device_slot,                                        \
    (device)->config_space.function,                                           \
    (device)->vendor_id,                                                       \
    (device)->id,                                                              \
    (device)->class,                                                           \
    (device)->subclass

#if defined(__x86_64__)
    bool
    pci_device_bind_msi_to_vector(struct pci_device_info *device,
                                  const struct cpu_info *cpu,
                                  isr_vector_t vector,
                                  bool masked);
#endif

enum pci_device_privilege {
    __PCI_DEVICE_PRIVL_PIO_ACCESS = 1ull << 0,
    __PCI_DEVICE_PRIVL_MEM_ACCESS = 1ull << 1,
    __PCI_DEVICE_PRIVL_BUS_MASTER = 1ull << 2,
};

void
pci_device_enable_privl(struct pci_device_info *device,
                        enum pci_device_privilege privl);