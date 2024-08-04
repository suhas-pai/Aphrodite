/*
 * kernel/src/arch/loongarch64/sys/isr.h
 * © suha spai
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
void isr_eoi(uint64_t intr_info);
