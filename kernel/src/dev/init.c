/*
 * kernel/src/dev/init.c
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
#include "pci/init.h"

#include "time/time.h"

void serial_init() {
#if defined(__x86_64__)
    com1_init();
#elif defined(__aarch64__)
    #if !defined(AARCH64_USE_16K_PAGES)
        const struct acpi_spcr *const spcr =
            (const struct acpi_spcr *)acpi_lookup_sdt("SPCR");

        uint64_t address = 0x9000000;
        uint32_t baudrate = 9600;
        uint8_t stop_bits = 0;

        if (spcr != NULL) {
            address = spcr->serial_port.address;
            baudrate = spcr->baud_rate;
            stop_bits = spcr->stop_bits;

            switch (spcr->baud_rate) {
                case ACPI_SPCR_BAUD_RATE_OS_DEPENDENT:
                    verify_not_reached();
                case ACPI_SPCR_BAUD_RATE_9600:
                    baudrate = 9600;
                    break;
                case ACPI_SPCR_BAUD_RATE_19200:
                    baudrate = 19200;
                    break;
                case ACPI_SPCR_BAUD_RATE_57600:
                    baudrate = 57600;
                    break;
                case ACPI_SPCR_BAUD_RATE_115200:
                    baudrate = 115200;
                    break;
            }

        }

        pl011_init((port_t)address, baudrate, /*data_bits=*/8, stop_bits);
    #endif /* !defined(AARCH64_USE_16K_PAGES) */
#elif defined(__riscv64)
    uart8250_init((port_t)0x10000000,
                  /*baudrate=*/115200,
                  /*in_freq=*/0,
                  /*reg_width=*/sizeof(uint8_t),
                  /*reg_shift=*/0);
#endif
}

void arch_init_dev();
void arch_init_time();

void dev_init() {
    acpi_init();

    arch_init_dev();
    arch_init_time();

    dtb_init();
    pci_init();

    printk(LOGLEVEL_INFO,
           "dev: initialized time, seconds since boot: %" PRIu64 "\n",
           nano_to_seconds(nsec_since_boot()));
}