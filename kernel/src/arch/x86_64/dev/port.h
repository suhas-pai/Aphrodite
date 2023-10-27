/*
 * kernel/arch/x86_64/dev/port.h
 * © suhas pai
 */

#pragma once
#include "sys/port.h"

enum port {
    PORT_PIT_CHANNEL_0_DATA = 0x40,
    PORT_PIT_MODE_COMMAND   = 0x43,

    PORT_PS2_READ_STATUS = 0x64,
    PORT_PS2_WRITE_CMD = PORT_PS2_READ_STATUS,
    PORT_PS2_INPUT_BUFFER = 0x60,

    PORT_CMOS_REGISTER_SELECT = 0x70,
    PORT_CMOS_REGISTER_READ = 0x71,

    PORT_PCI_CONFIG_ADDRESS = 0xCF8,
    PORT_PCI_CONFIG_DATA = 0xCFC,

    PORT_QEMU_SHUTDOWN = 0x604
};