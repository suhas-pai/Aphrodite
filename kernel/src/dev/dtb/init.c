/*
 * kernel/src/dev/dtb/init.c
 * © suhas pai
 */

#include "dev/pci/pci.h"

#if defined(__riscv64)
    #include "dev/uart/8250.h"
#endif /* defined(__riscv64) */

#include "dev/driver.h"
#include "dev/printk.h"
#include "fdt/libfdt.h"

#include "sys/boot.h"
#include "dtb.h"

#if defined(__riscv64)
    static void init_serial_device(const void *const dtb, const int nodeoff) {
        struct dtb_addr_size_pair base_addr_reg = DTB_ADDR_SIZE_PAIR_INIT();
        uint32_t pair_count = 1;

        const bool get_base_addr_reg_result =
            dtb_get_reg_pairs(dtb,
                              nodeoff,
                              /*start_index=*/0,
                              &pair_count,
                              &base_addr_reg);

        if (!get_base_addr_reg_result) {
            panic("dtb: base-addr reg of 'reg' property of serial node is "
                  "malformed\n");
        }

        struct string_view clock_freq_string = SV_EMPTY();
        const bool get_clock_freq_result =
            dtb_get_string_prop(dtb,
                                nodeoff,
                                "clock-frequency",
                                &clock_freq_string);

        if (!get_clock_freq_result) {
            panic("dtb: clock-frequency property of serial node is missing or "
                  "malformed\n");
        }

        uart8250_init((port_t)base_addr_reg.address,
                      /*baudrate=*/115200,
                      *(uint32_t *)(uint64_t)clock_freq_string.begin,
                      /*reg_width=*/sizeof(uint8_t),
                      /*reg_shift=*/0);
    }
#endif /* defined(__riscv64) */

static void init_pci_node(const void *const dtb, const int pci_offset) {
    struct string_view device_type = SV_EMPTY();
    const bool get_compat_result =
        dtb_get_string_prop(dtb, pci_offset, "device_type", &device_type);

    if (!get_compat_result) {
        printk(LOGLEVEL_WARN,
               "dtb: \"device_type\" property of pci node is missing");
        return;
    }

    if (sv_compare_c_str(device_type, "pci") != 0) {
        printk(LOGLEVEL_WARN,
               "dtb: \"compatible\" property of pci node is not pci, got "
               SV_FMT " instead\n",
               SV_FMT_ARGS(device_type));
        return;
    }

    const fdt32_t *bus_range_array = NULL;
    uint32_t bus_range_length = 0;

    const bool get_bus_range_result =
        dtb_get_array_prop(dtb,
                           pci_offset,
                           "bus-range",
                           &bus_range_array,
                           &bus_range_length);

    if (!get_bus_range_result) {
        printk(LOGLEVEL_INFO,
               "dtb: 'bus-range' property of pci node is either missing or "
               "malformed\n");
        return;
    }

    if (bus_range_length != 2) {
        printk(LOGLEVEL_INFO,
               "dtb: 'bus-range' property of pci node is malformed\n");
        return;
    }

    const fdt32_t *domain_array = NULL;
    uint32_t domain_array_length = 0;

    const bool get_domain_result =
        dtb_get_array_prop(dtb,
                           pci_offset,
                           "linux,pci-domain",
                           &domain_array,
                           &domain_array_length);

    if (!get_domain_result) {
        printk(LOGLEVEL_INFO,
               "dtb: domain property of pci node is either missing or "
               "malformed\n");
        return;
    }

    if (domain_array_length != 1) {
        printk(LOGLEVEL_INFO,
               "dtb: domain property of pci node is malformed\n");
        return;
    }

    struct dtb_addr_size_pair base_addr_reg = DTB_ADDR_SIZE_PAIR_INIT();
    uint32_t base_addr_pair_count = 1;

    const bool get_base_addr_reg_result =
        dtb_get_reg_pairs(dtb,
                          pci_offset,
                          /*start_index=*/0,
                          &base_addr_pair_count,
                          &base_addr_reg);

    if (!get_base_addr_reg_result) {
        printk(LOGLEVEL_INFO,
               "dtb: base-addr reg of 'reg' property of pci node is "
               "malformed\n");
        return;
    }

    const struct range bus_range =
        RANGE_INIT(fdt32_to_cpu(bus_range_array[0]),
                   fdt32_to_cpu(bus_range_array[1]));

    pci_add_pcie_domain(bus_range,
                        base_addr_reg.address,
                        /*segment=*/fdt32_to_cpu(domain_array[0]));
}

static void init_pci_drivers(const void *const dtb) {
    for_each_dtb_compat(dtb, offset, "pci-host-ecam-generic") {
        init_pci_node(dtb, offset);
    }

    for_each_dtb_compat(dtb, offset, "pci-host-cam-generic") {
        init_pci_node(dtb, offset);
    }
}

#if defined(__riscv64)
    static void init_riscv_serial_devices(const void *const dtb) {
        for_each_dtb_compat(dtb, offset, "ns16550a") {
            init_serial_device(dtb, offset);
        }
    }
#endif /* defined(__riscv64) */

void dtb_init_early() {
#if defined(__riscv64)
    const void *const dtb = boot_get_dtb();
    if (dtb != NULL) {
        init_riscv_serial_devices(dtb);
    }

    printk(LOGLEVEL_INFO, "dtb: finished early init\n");
#endif /* defined(__riscv64) */
}

void
dtb_init_nodes_for_driver(const void *const dtb,
                          struct dtb_driver *const driver)
{
    for (uint32_t i = 0; i != driver->compat_count; i++) {
        const char *const compat = driver->compat_list[i];
    #if defined(__riscv64)
        if (strcmp(compat, "ns16550a") == 0) {
            continue;
        }
    #endif /* defined(__riscv64) */

        for_each_dtb_compat(dtb, offset, compat) {
            driver->init(dtb, offset);
        }
    }
}

void dtb_init() {
    const void *const dtb = boot_get_dtb();
    if (dtb == NULL) {
        printk(LOGLEVEL_WARN, "dev: dtb not found\n");
        return;
    }

    init_pci_drivers(dtb);
    driver_foreach(driver) {
        if (driver->dtb != NULL) {
            dtb_init_nodes_for_driver(dtb, driver->dtb);
        }
    }

    printk(LOGLEVEL_INFO, "dtb: finished initializing\n");
}
