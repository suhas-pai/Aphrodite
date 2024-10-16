/*
 * kernel/src/sys/irq.h
 * © suhas pai
 */

#pragma once
#include "sys/irqdef.h"

enum irq_polarity {
    IRQ_POLARITY_LOW,
    IRQ_POLARITY_HIGH,
};

enum irq_trigger_mode {
    IRQ_TRIGGER_MODE_EDGE,
    IRQ_TRIGGER_MODE_LEVEL,
};
