/*
 * kernel/dev/init.c
 * © suhas pai
 */

#if defined(__x86_64__)
    #include "dev/uart/com1.h"
#elif defined(__aarch64__)
    #include "dev/uart/pl011.h"
#elif defined(__riscv64)
    #include "dev/uart/8250.h"
#endif /* defined(__x86_64__) */

#include "acpi/api.h"
#include "dtb/init.h"

#include "dev/printk.h"
#include "pci/pci.h"
#include "time/time.h"

void serial_init() {
#if defined(__x86_64__)
    com1_init();
#elif defined(__aarch64__)
    pl011_init((port_t)0x9000000,
               /*baudrate=*/115200,
               /*data_bits=*/8,
               /*stop_bits=*/1);
#elif defined(__riscv64)
    uart8250_init((port_t)0x10000000,
                  /*baudrate=*/115200,
                  /*in_freq=*/0,
                  /*reg_width=*/sizeof(uint8_t),
                  /*reg_shift=*/0);
#endif

    dtb_init_early();
}

void arch_init_dev();
void dev_init() {
    acpi_init();
    arch_init_time();

    dtb_init();
    pci_init();

    arch_init_dev();
    printk(LOGLEVEL_INFO,
           "dev: initialized time, seconds since boot: %" PRIu64 "\n",
           nano_to_seconds(nsec_since_boot()));
}