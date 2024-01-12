/*
 * kernel/src/arch/x86_64/dev/cmos.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "asm/pause.h"

#include "cmos.h"
#include "pio.h"

static inline void select_cmos_register(enum cmos_register reg) {
    reg &= (unsigned)~0x80;
    pio_write8(PIO_PORT_CMOS_REGISTER_SELECT, reg);

    /*
     * According to wiki.osdev.org;
     *   It is probably a good idea to have a reasonable delay after selecting
     *   a CMOS register on Port 0x70, before reading/writing the value on Port
     *   0x71. "
     */

    cpu_pause();
}

uint8_t cmos_read(const enum cmos_register reg) {
    const bool flag = disable_interrupts_if_not();

    // Note: We have to select the cmos register every time as reading/writing
    // from cmos deselects cmos.

    select_cmos_register(reg);
    const uint8_t result = pio_read8(PIO_PORT_CMOS_REGISTER_READ);

    enable_interrupts_if_flag(flag);
    return result;
}

void cmos_write(const enum cmos_register reg, const uint8_t data) {
    const bool flag = disable_interrupts_if_not();

    select_cmos_register((uint8_t)reg);
    pio_write8((uint8_t)reg, data);

    enable_interrupts_if_flag(flag);
}