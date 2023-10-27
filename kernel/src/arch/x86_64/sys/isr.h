/*
 * kernel/arch/x86_64/sys/isr.h
 * © suhas pai
 */

#pragma once

#include "asm/irq_context.h"
#include "sys/idt.h"

struct arch_isr_info {
    uint8_t ist;
};

#define ARCH_ISR_INFO_NONE() \
    ((struct arch_isr_info){ \
        .ist = IST_NONE \
    })

typedef idt_vector_t isr_vector_t;
#define ISR_VECTOR_FMT "%" PRIu8

typedef void (*isr_func_t)(uint64_t int_no, irq_context_t *frame);

isr_vector_t isr_get_spur_vector();
isr_vector_t isr_get_timer_vector();