/*
 * kernel/arch/riscv64/sys/isr.h
 * © suhas pai
 */

#pragma once
#include "asm/irq_context.h"

struct arch_isr_info {};

typedef uint16_t isr_vector_t;
#define ISR_VECTOR_FMT "%" PRIu16

typedef void (*isr_func_t)(uint64_t int_no, irq_context_t *frame);