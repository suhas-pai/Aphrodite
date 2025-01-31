/*
 * kernel/src/acpi/spcr.c
 * © suhas pai
 */

#include "acpi/spcr.h"
#include "dev/printk.h"

void spcr_init(const struct os_acpi_spcr *const spcr) {
    if (spcr->sdt.length < sizeof(*spcr)) {
        printk(LOGLEVEL_WARN, "spcr: table too short\n");
        return;
    }

    const char *interface_kind_str = "unknown";
    switch (spcr->interface_kind) {
        case OS_ACPI_SPCR_INTERFACE_16550_COMPATIBLE:
            interface_kind_str = "16550-compatible";
            break;
        case OS_ACPI_SPCR_INTERFACE_16550_SUBSET:
            interface_kind_str = "16550-subset";
            break;
        case OS_ACPI_SPCR_INTERFACE_MAX311XE_SPI:
            interface_kind_str = "max311xe-spi";
            break;
        case OS_ACPI_SPCR_INTERFACE_ARM_PL011:
            interface_kind_str = "arm-pl011";
            break;
        case OS_ACPI_SPCR_INTERFACE_MSM8X60:
            interface_kind_str = "msm8x60";
            break;
        case OS_ACPI_SPCR_INTERFACE_16550_NVIDIA:
            interface_kind_str = "16550-nvidia";
            break;
        case OS_ACPI_SPCR_INTERFACE_TI_OMAP:
            interface_kind_str = "ti-omap";
            break;
        case OS_ACPI_SPCR_INTERFACE_APM88XXXX:
            interface_kind_str = "apm-88xxxx";
            break;
        case OS_ACPI_SPCR_INTERFACE_MSM8974:
            interface_kind_str = "msm-8974";
            break;
        case OS_ACPI_SPCR_INTERFACE_SAM5250:
            interface_kind_str = "sam5250";
            break;
        case OS_ACPI_SPCR_INTERFACE_INTEL_USIF:
            interface_kind_str = "intel-usif";
            break;
        case OS_ACPI_SPCR_INTERFACE_IMX6:
            interface_kind_str = "imx6";
            break;
        case OS_ACPI_SPCR_INTERFACE_ARM_SBSA_32BIT:
            interface_kind_str = "arm-sbsa-32b";
            break;
        case OS_ACPI_SPCR_INTERFACE_ARM_SBSA_GENERIC:
            interface_kind_str = "arm-sbsa-generic";
            break;
        case OS_ACPI_SPCR_INTERFACE_ARM_DCC:
            interface_kind_str = "arm-dcc";
            break;
        case OS_ACPI_SPCR_INTERFACE_BCM2835:
            interface_kind_str = "bcm-2835";
            break;
        case OS_ACPI_SPCR_INTERFACE_SDM845_1_8432MHZ:
            interface_kind_str = "sdm845-1-8432mhz";
            break;
        case OS_ACPI_SPCR_INTERFACE_16550_WITH_GAS:
            interface_kind_str = "16550-with-gas";
            break;
        case OS_ACPI_SPCR_INTERFACE_SDM845_7_372MHZ:
            interface_kind_str = "sdm845-7-372mhz";
            break;
        case OS_ACPI_SPCR_INTERFACE_INTEL_LPSS:
            interface_kind_str = "intel-lpss";
            break;
    }

    const char *baud_rate_str = "unknown";
    switch (spcr->baud_rate) {
        case OS_ACPI_SPCR_BAUD_RATE_OS_DEPENDENT:
            baud_rate_str = "os-dependent";
            break;
        case OS_ACPI_SPCR_BAUD_RATE_9600:
            baud_rate_str = "9600";
            break;
        case OS_ACPI_SPCR_BAUD_RATE_19200:
            baud_rate_str = "19200";
            break;
        case OS_ACPI_SPCR_BAUD_RATE_57600:
            baud_rate_str = "57600";
            break;
        case OS_ACPI_SPCR_BAUD_RATE_115200:
            baud_rate_str = "115200";
            break;
    }

    const char *terminal_kind_str = "unknown";
    switch (spcr->terminal_kind) {
        case OS_ACPI_SPCR_TERMINAL_VT100:
            terminal_kind_str = "vt100";
            break;
        case OS_ACPI_SPCR_TERMINAL_VT100_EXT:
            terminal_kind_str = "vt100-extended";
            break;
        case OS_ACPI_SPCR_TERMINAL_VT_UTF8:
            terminal_kind_str = "vt-utf8";
            break;
        case OS_ACPI_SPCR_TERMINAL_ANSI:
            terminal_kind_str = "ansi";
            break;
    }

    printk(LOGLEVEL_INFO,
           "spcr:\n"
           "\t\t" "interface kind: %s\n"
           "\t\t" "base address:\n"
           "\t\t\t" "addr space: %" PRIu8 "\n"
           "\t\t\t" "bit width: %" PRIu8 "\n"
           "\t\t\t" "bit offset: 0x%" PRIx8 "\n"
           "\t\t\t" "access size: %" PRIu8 "\n"
           "\t\t\t" "address: 0x%" PRIx64 "\n"
           "\t\t" "interrupt kind: 0x%" PRIx8 "\n"
           "\t\t\t" "8259 PIC: %s\n"
           "\t\t\t" "io/apic: %s\n"
           "\t\t\t" "io/sapic: %s\n"
           "\t\t\t" "arm gic: %s\n"
           "\t\t\t" "risc-v plic: %s\n"
           "\t\t" "pc interrupt: %" PRIu8 "\n"
           "\t\t" "global system interrupt: %" PRIu32 "\n"
           "\t\t" "baud rate: %s\n"
           "\t\t" "parity: %" PRIu8 "\n"
           "\t\t" "stop bits: %" PRIu8 "\n"
           "\t\t" "flow control: 0x%" PRIx8 "\n"
           "\t\t" "terminal kind: %s\n"
           "\t\t" "pci device-id: 0x%" PRIx8 "\n"
           "\t\t" "pci vendor-id: 0x%" PRIx8 "\n"
           "\t\t" "pci bus: 0x%" PRIx8 "\n"
           "\t\t" "pci device: 0x%" PRIx8 "\n"
           "\t\t" "pci function: 0x%" PRIx8 "\n"
           "\t\t" "pci flags: 0x%" PRIx32 "\n"
           "\t\t" "pci segment: 0x%" PRIx32 "\n"
           "\t\t" "uart clock-frequency: 0x%" PRIx32 "\n",
           interface_kind_str,
           spcr->serial_port.addr_space,
           spcr->serial_port.bit_width,
           spcr->serial_port.bit_offset,
           spcr->serial_port.access_size,
           spcr->serial_port.address,
           spcr->interrupt_kind,
           spcr->interrupt_kind & __OS_ACPI_SPCR_IRQ_8259 ? "yes" : "no",
           spcr->interrupt_kind & __OS_ACPI_SPCR_IRQ_IOAPIC ? "yes" : "no",
           spcr->interrupt_kind & __OS_ACPI_SPCR_IRQ_IO_SAPIC ? "yes" : "no",
           spcr->interrupt_kind & __OS_ACPI_SPCR_IRQ_ARM_GIC ? "yes" : "no",
           spcr->interrupt_kind & __OS_ACPI_SPCR_IRQ_RISCV_PLIC ? "yes" : "no",
           spcr->pc_interrupt,
           spcr->gsiv,
           baud_rate_str,
           spcr->parity,
           spcr->stop_bits,
           spcr->flow_control,
           terminal_kind_str,
           spcr->pci_device_id,
           spcr->pci_vendor_id,
           spcr->pci_bus,
           spcr->pci_device,
           spcr->pci_function,
           spcr->pci_flags,
           spcr->pci_segment,
           spcr->uart_clock_freq);
}