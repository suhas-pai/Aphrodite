/*
 * kernel/src/arch/x86_64/dev/cmos.c
 * © suhas pai
 */

#include "asm/irqs.h"
#include "asm/pause.h"

#include "cmos.h"
#include "pio.h"

__debug_optimize(3)
static inline void select_cmos_register(const enum cmos_register reg) {
    pio_write8(PIO_PORT_CMOS_REGISTER_SELECT, rm_mask(reg, 1 << 7));

    /*
     * According to wiki.osdev.org;
     *   It is probably a good idea to have a reasonable delay after selecting
     *   a CMOS register on Port 0x70, before reading/writing the value on Port
     *   0x71. "
     */
    cpu_pause();
}

__debug_optimize(3) uint8_t cmos_read(const enum cmos_register reg) {
    uint8_t result = 0;
    WITH_IRQS_DISABLED({
        // Note: We have to select the cmos register every time as
        // reading/writing from cmos deselects cmos.

        select_cmos_register(reg);
        result = pio_read8(PIO_PORT_CMOS_REGISTER_READ);
    });

    return result;
}

__debug_optimize(3)
void cmos_write(const enum cmos_register reg, const uint8_t data) {
    WITH_IRQS_DISABLED({
        select_cmos_register((uint8_t)reg);
        pio_write8((uint8_t)reg, data);
    });
}