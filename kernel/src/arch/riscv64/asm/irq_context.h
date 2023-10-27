/*
 * kernel/arch/riscv64/asm/irq_context.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct riscv_irq_context {
    uint32_t ra;
    uint32_t sp;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
};

typedef struct riscv_irq_context irq_context_t;