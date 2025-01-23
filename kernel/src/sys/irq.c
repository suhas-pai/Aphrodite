/*
 * kernel/src/sys/irq.c
 * © suhas pai
 */

#include "sys/irq.h"

void
irq_pin_setup(struct irq_pin *const pin,
              const enum irq_polarity polarity,
              const enum irq_trigger_mode trigger_mode)
{
    if (pin->initialized) {
        if (polarity != pin->polarity && trigger_mode != pin->trigger_mode) {
            panic("irq: polarity and trigger-mode mismatch for irq %d\n"
                  "\tpolarity: have %s vs desired %s\n"
                  "\ttrigger-mode: have %s vs desired %s\n",
                  pin->irq,
                  pin->polarity == IRQ_POLARITY_LOW ? "low" : "high",
                  polarity == IRQ_POLARITY_LOW ? "low" : "high",
                  pin->trigger_mode == IRQ_TRIGGER_MODE_LEVEL ?
                   "level" : "edge",
                  trigger_mode == IRQ_TRIGGER_MODE_LEVEL ? "level" : "edge");
        }

        assert_msg(pin->polarity == polarity,
                   "irq: polarity mismatch for irq %d\n"
                   "\tpolarity: have %s vs desired %s\n",
                   pin->irq,
                   pin->polarity == IRQ_POLARITY_LOW ? "low" : "high",
                   polarity == IRQ_POLARITY_LOW ? "low" : "high");

        assert_msg(pin->trigger_mode == trigger_mode,
                   "irq: trigger-mode mismatch for irq %d\n"
                   "\ttrigger-mode: have %s vs desired %s\n",
                   pin->irq,
                   pin->trigger_mode == IRQ_TRIGGER_MODE_LEVEL ?
                    "level" : "edge",
                   trigger_mode == IRQ_TRIGGER_MODE_LEVEL ? "level" : "edge");

        return;
    }

    pin->polarity = polarity;
    pin->trigger_mode = trigger_mode;

    pin->initialized = true;
}
