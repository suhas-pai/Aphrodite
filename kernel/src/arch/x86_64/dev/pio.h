/*
 * kernel/src/arch/x86_64/dev/port.h
 * © suhas pai
 */

#pragma once
#include "sys/pio.h"

enum pio_port {
    PIO_PORT_PIT_CHANNEL_0_DATA = 0x40,
    PIO_PORT_PIT_MODE_COMMAND = 0x43,

    PIO_PORT_PS2_READ_STATUS = 0x64,
    PIO_PORT_PS2_WRITE_CMD = PIO_PORT_PS2_READ_STATUS,
    PIO_PORT_PS2_INPUT_BUFFER = 0x60,

    PIO_PORT_CMOS_REGISTER_SELECT = 0x70,
    PIO_PORT_CMOS_REGISTER_READ = 0x71,

    PIO_PORT_PCI_CONFIG_ADDRESS = 0xCF8,
    PIO_PORT_PCI_CONFIG_DATA = 0xCFC,

    PIO_PORT_QEMU_SHUTDOWN = 0x604
};