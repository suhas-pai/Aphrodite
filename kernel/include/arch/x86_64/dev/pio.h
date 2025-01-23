/*
 * kernel/include/arch/x86_64/dev/pio.h
 * © suhas pai
 */

#pragma once
#include "sys/pio.h"

enum pio_port : uint16_t {
    PIO_PORT_PIT_CHANNEL_0_DATA = 0x40,
    PIO_PORT_PIT_MODE_COMMAND = 0x43,

    PIO_PORT_CMOS_REGISTER_SELECT = 0x70,
    PIO_PORT_CMOS_REGISTER_READ = 0x71,

    PIO_PORT_PCI_CONFIG_ADDRESS = 0xCF8,
    PIO_PORT_PCI_CONFIG_DATA = 0xCFC,

    PIO_PORT_QEMU_SHUTDOWN = 0x604
};
