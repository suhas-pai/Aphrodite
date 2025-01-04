/*
 * kernel/src/arch/x86_64/dev/pit.c
 * © suhas pai
 */

#include "asm/context.h"
#include "asm/irqs.h"

#include "cpu/isr.h"

#include "dev/pio.h"
#include "dev/pit.h"

#include "dev/printk.h"
#include "sched/thread.h"

#define PIT_DIVIDEND 1193180

static uint64_t g_tick = 0;
static enum pit_granularity g_gran = 0;

// TODO: Implement callbacks, sleep, etc.
void
irq$pit(const uint64_t intr_no,
        struct thread_context *const regs,
        void *const ctx)
{
    (void)intr_no;
    (void)regs;
    (void)ctx;

    g_tick++;
    if ((g_tick % 1000) == 0) {
        printk(LOGLEVEL_INFO, "pit: tick at %" PRIu64 "\n", g_tick);
    }

    isr_eoi(intr_no);
}

#define PIT_IRQ 0

void pit_init(const uint8_t flags, const enum pit_granularity granularity) {
    g_gran = granularity;

    struct irq_pin *const pin = isr_get_irq_pin(PIT_IRQ);
    isr_install_irq(pin, irq$pit, /*ctx=*/nullptr, /*masked=*/false);

    const uint32_t divisor = PIT_DIVIDEND / (uint32_t)granularity;
    const uint8_t data = ((divisor >> 8) & 0xFF);

    pio_write8(PIO_PORT_PIT_MODE_COMMAND, flags);
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, (divisor & 0xFF));
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, data);
}

__debug_optimize(3) void pit_sleep_for(const uint32_t ms) {
    with_intr_disabled({
        pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0x30);
        pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, ms);
    });

    do {
        uint8_t status = 0;
        with_intr_disabled({
            pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0xE2);
            status = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);
        });

        if (status & 1 << 7) {
            break;
        }
    } while (true);
}

__debug_optimize(3) uint16_t pit_get_current_tick() {
    uint8_t low = 0;
    uint8_t high = 0;

    with_intr_disabled({
        pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0);

        low = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);
        high = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);
    });

    return (uint16_t)high << 8 | low;
}

__debug_optimize(3) void pit_set_reload_value(const uint16_t count) {
    with_intr_disabled({
        pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, count & 0xFF);
        pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, (count & 0xFF00) >> 8);
    });
}