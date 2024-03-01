/*
 * kernel/src/arch/x86_64/dev/pit.c
 * © suhas pai
 */

#include "asm/context.h"
#include "asm/irqs.h"

#include "dev/printk.h"

#include "pio.h"
#include "pit.h"

static uint64_t g_tick = 0;
static enum pit_granularity g_gran = 0;

// TODO: Implement callbacks, sleep, etc.
void irq$pit(const uint64_t intr_no, struct thread_context *const regs) {
    (void)intr_no;
    (void)regs;

    printk(LOGLEVEL_INFO, "pit: tick at %" PRIu64, g_tick);
    g_tick++;
}

void pit_init(const uint8_t flags, const enum pit_granularity granularity) {
    g_gran = granularity;

    const uint32_t divisor = 1193180 / (uint32_t)granularity;
    const uint8_t data = ((divisor >> 8) & 0xFF);

    pio_write8(PIO_PORT_PIT_MODE_COMMAND, flags);
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, (divisor & 0xFF));
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, data);

    //isr_register_for_vector(IRQ_TIMER, irq$pit);
}

__optimize(3) void pit_sleep_for(const uint32_t ms) {
    pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0x30);
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, ms);

    do {
        pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0xE2);
        const uint8_t status = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);

        if (status & 1 << 7) {
            break;
        }
    } while (true);
}

__optimize(3) uint16_t pit_get_current_tick() {
    const bool flag = disable_interrupts_if_not();
    pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0);

    const uint8_t low = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);
    const uint8_t high = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);

    enable_interrupts_if_flag(flag);
    return (uint16_t)high << 8 | low;
}

__optimize(3) void pit_set_reload_value(const uint16_t count) {
    const bool flag = disable_interrupts_if_not();

    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, count & 0xFF);
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, (count & 0xFF00) >> 8);

    enable_interrupts_if_flag(flag);
}