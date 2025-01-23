/*
 * kernel/include/sys/irq.h
 * © suhas pai
 */

#pragma once

#include "cpu/isr.h"
#include "sys/irqdef.h"

enum irq_polarity : uint8_t {
    IRQ_POLARITY_LOW,
    IRQ_POLARITY_HIGH,
};

enum irq_trigger_mode : uint8_t {
    IRQ_TRIGGER_MODE_EDGE,
    IRQ_TRIGGER_MODE_LEVEL,
};

struct irq_pin {
    irq_number_t irq;
    isr_vector_t vector;

    enum irq_polarity polarity : 1;
    enum irq_trigger_mode trigger_mode : 1;

    bool initialized : 1;
};

void
irq_pin_setup(struct irq_pin *pin,
              enum irq_polarity polarity,
              enum irq_trigger_mode trigger_mode);
