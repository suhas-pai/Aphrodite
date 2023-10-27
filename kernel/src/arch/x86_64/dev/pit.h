/*
 * kernel/arch/x86_64/dev/pit.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

enum pit_timer_channel {
    PIT_TIMER_CHANNEL_0 = 0,
    PIT_TIMER_CHANNEL_1 = (1 << 6),
    PIT_TIMER_CHANNEL_2 = (1 << 7),
    PIT_TIMER_CHANNEL_3 = (1 << 7) | (1 << 6)
};

// The PIT has a frequency of 1193180 Hz or 1.1931816666 MHz
#define PIT_FREQUENCY 1193180
#define PIT_TIMER_CHANNEL_MASK ((1 << 7) | (1 << 6))

enum pit_timer_access_byte {
    PIT_TIMER_ACCESS_MODE_LATCH     = 0,
    PIT_TIMER_ACCESS_MODE_LO_BYTE   = (1 << 4),
    PIT_TIMER_ACCESS_MODE_HI_BYTE   = (1 << 5),
    PIT_TIMER_ACCESS_MODE_FULL_BYTE =
        (PIT_TIMER_ACCESS_MODE_HI_BYTE | PIT_TIMER_ACCESS_MODE_LO_BYTE)
};

enum pit_timer_op_mode {
    PIT_TIMER_OP_MODE_0,
    PIT_TIMER_OP_MODE_INTERRUPT_TERMINAL_COUNT = PIT_TIMER_OP_MODE_0,

    PIT_TIMER_OP_MODE_1 = (1 << 1),
    PIT_TIMER_OP_MODE_HW_RETRIGGABLE_ONE_SHOT = PIT_TIMER_OP_MODE_1,

    PIT_TIMER_OP_MODE_2 = (2 << 1),
    PIT_TIMER_OP_MODE_RATE_GENERATOR = PIT_TIMER_OP_MODE_2,

    PIT_TIMER_OP_MODE_3 = (3 << 1),
    PIT_TIMER_OP_MODE_SQUARE_WAVE = PIT_TIMER_OP_MODE_3,

    PIT_TIMER_OP_MODE_4 = (4 << 1),
    PIT_TIMER_OP_MODE_SW_TRIGGERED_STROBE = PIT_TIMER_OP_MODE_4,

    PIT_TIMER_OP_MODE_5 = (5 << 1),
    PIT_TIMER_OP_MODE_HW_TRIGGERED_STROBE = PIT_TIMER_OP_MODE_5
};

enum pit_granularity {
    PIT_GRANULARITY_1_MS    = 1000,
    PIT_GRANULARITY_5_MS    = 1000,
    PIT_GRANULARITY_10_MS   = 200,
    PIT_GRANULARITY_20_MS   = 50,
    PIT_GRANULARITY_50_MS   = 20,
    PIT_GRANULARITY_100_MS  = 10,
    PIT_GRANULARITY_500_MS  = 2,
    PIT_GRANULARITY_1000_MS = 1
};

_Static_assert(
    (
        PIT_TIMER_CHANNEL_0 |
        PIT_TIMER_ACCESS_MODE_FULL_BYTE |
        PIT_TIMER_OP_MODE_SQUARE_WAVE
    ) == 0x36, "");

void pit_init(uint8_t flags, enum pit_granularity granularity);
void pit_sleep_for(uint32_t ms);

uint16_t pit_get_current_tick();
void pit_set_reload_value(uint16_t value);