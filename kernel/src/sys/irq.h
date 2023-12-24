/*
 * kernel/src/sys/irq.h
 * © suhas pai
 */

#pragma once

enum irq_polarity {
    IRQ_POLARITY_LOW,
    IRQ_POLARITY_HIGH,
};

enum irq_trigger_mpde {
    IRQ_TRIGGER_MODE_EDGE,
    IRQ_TRIGGER_MODE_LEVEL,
};