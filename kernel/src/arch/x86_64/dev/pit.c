/*
 * kernel/src/arch/x86_64/dev/pit.c
 * © suhas pai
 */

#include "asm/context.h"
#include "asm/irqs.h"

#include "cpu/isr.h"
#include "dev/printk.h"

#include "pio.h"
#include "pit.h"

#define PIT_DIVIDEND 1193180

static uint64_t g_tick = 0;
static enum pit_granularity g_gran = 0;

// TODO: Implement callbacks, sleep, etc.
void irq$pit(const uint64_t intr_no, struct thread_context *const regs) {
    (void)intr_no;
    (void)regs;

    g_tick++;
    if ((g_tick % 1000) == 0) {
        printk(LOGLEVEL_INFO, "pit: tick at %" PRIu64 "\n", g_tick);
    }

    isr_eoi(intr_no);
}

#define PIT_IRQ 0

void pit_init(const uint8_t flags, const enum pit_granularity granularity) {
    g_gran = granularity;

    const isr_vector_t vector = isr_alloc_vector();
    assert(vector != ISR_INVALID_VECTOR);

    WITH_IRQS_DISABLED({
        isr_assign_irq_to_cpu(this_cpu(), PIT_IRQ, vector, /*masked=*/false);
    });

    isr_set_vector(vector, irq$pit, &ARCH_ISR_INFO_NONE());

    const uint32_t divisor = PIT_DIVIDEND / (uint32_t)granularity;
    const uint8_t data = ((divisor >> 8) & 0xFF);

    pio_write8(PIO_PORT_PIT_MODE_COMMAND, flags);
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, (divisor & 0xFF));
    pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, data);
}

__debug_optimize(3) void pit_sleep_for(const uint32_t ms) {
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

__debug_optimize(3) uint16_t pit_get_current_tick() {
    uint8_t low = 0;
    uint8_t high = 0;

    WITH_IRQS_DISABLED({
        pio_write8(PIO_PORT_PIT_MODE_COMMAND, 0);

        low = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);
        high = pio_read8(PIO_PORT_PIT_CHANNEL_0_DATA);
    });

    return (uint16_t)high << 8 | low;
}

__debug_optimize(3) void pit_set_reload_value(const uint16_t count) {
    WITH_IRQS_DISABLED({
        pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, count & 0xFF);
        pio_write8(PIO_PORT_PIT_CHANNEL_0_DATA, (count & 0xFF00) >> 8);
    });
}