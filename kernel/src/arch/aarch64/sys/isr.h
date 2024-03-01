/*
 * kernel/src/arch/aarch64/sys/isr.h
 * © suhas pai
 */

#pragma once
#include "asm/context.h"

struct arch_isr_info {};
#define ARCH_ISR_INFO_NONE() ((struct arch_isr_info){})

typedef uint16_t isr_vector_t;

#define ISR_SUPPORTS_MSI 1
#define ISR_VECTOR_FMT "%" PRIu16
#define ISR_INVALID_VECTOR UINT16_MAX

typedef void (*isr_func_t)(uint64_t intr_info, struct thread_context *frame);
void isr_reserve_msi_irqs(uint16_t base, uint16_t count);

void isr_install_vbar();
void isr_eoi(uint64_t intr_info);
