/*
 * kernel/src/arch/x86_64/dev/ahci/irq.h
 * © suhas pai
 */

#pragma once
#include "sys/isr.h"

void ahci_port_handle_irq(uint64_t int_no, irq_context_t *frame);
