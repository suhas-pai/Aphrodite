/*
 * kernel/src/arch/x86_64/sys/isr.h
 * © suhas pai
 */

#pragma once

#include "asm/context.h"
#include "sys/idt.h"

struct arch_isr_info {
    uint8_t ist;
};

#define ARCH_ISR_INFO_NONE() \
    ((struct arch_isr_info){ \
        .ist = IST_NONE \
    })

typedef idt_vector_t isr_vector_t;

#define ISR_SUPPORTS_MSI 1
#define ISR_VECTOR_FMT "%" PRIu8
#define ISR_INVALID_VECTOR UINT8_MAX

typedef void (*isr_func_t)(uint64_t intr_no, struct thread_context *frame);

isr_vector_t isr_get_spur_vector();
isr_vector_t isr_get_lapic_vector();
isr_vector_t isr_get_hpet_vector();
