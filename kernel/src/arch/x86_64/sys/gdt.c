/*
 * kernel/arch/x86_64/sys/gdt.c
 * © suhas pai
 */

#include <stdint.h>
#include "lib/macros.h"

struct gdt_descriptor {
    uint16_t limit;
    uint16_t base_low16;
    uint8_t  base_mid8;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high8;
};

struct tss_descriptor {
    uint16_t length;
    uint16_t base_low16;
    uint8_t  base_mid8;
    uint8_t  flags1;
    uint8_t  flags2;
    uint8_t  base_high8;
    uint32_t base_upper32;
    uint32_t reserved;
};

#define ACC_CODE_READABLE (1 << 1)
#define ACC_DATA_WRITABLE ACC_CODE_READABLE

#define ACC_DATA_DOWN (1 << 2)
#define ACC_CODE_RING_CONFIRM ACC_DATA_DOWN

#define ACC_CODE_SEG (1 << 3)
#define ACC_NON_TSS_SEG (1 << 4)

#define ACC_DPL3 (3 << 5)
#define ACC_PRESENT (1 << 7)

#define ACC_KERNEL_CODE (ACC_CODE_SEG | ACC_CODE_READABLE | ACC_NON_TSS_SEG)
#define ACC_KERNEL_DATA (ACC_DATA_WRITABLE | ACC_NON_TSS_SEG)

#define ACC_USER_CODE \
    (ACC_CODE_SEG | ACC_CODE_READABLE | ACC_NON_TSS_SEG | ACC_DPL3)
#define ACC_USER_DATA (ACC_DATA_WRITABLE | ACC_NON_TSS_SEG | ACC_DPL3)

_Static_assert(ACC_KERNEL_CODE == 0b00011010, "ACC_KERNEL_CODE isn't correct");
_Static_assert(ACC_KERNEL_DATA == 0b00010010, "ACC_KERNEL_CODE isn't correct");

_Static_assert(ACC_USER_CODE == 0b01111010, "ACC_USER_CODE isn't correct");
_Static_assert(ACC_USER_DATA == 0b01110010, "ACC_DATA_CODE isn't correct");

#define GRAN_LIMIT_HIGH 0b10001111
#define GRAN_LONG (1 << 5)
#define GRAN_32_BIT_SEG (1 << 6)

#define GRAN_32 ((uint8_t)(GRAN_32_BIT_SEG | GRAN_LIMIT_HIGH))
#define GRAN_64 ((uint8_t)GRAN_LONG)

_Static_assert(GRAN_32 == 0b11001111, "GRAN_32 is incorrect");
_Static_assert(GRAN_64 == 0b00100000, "GRAN_64 is incorrect");

#define TSS_64 0b1001
#define TSS_PRESENT 0b10000000

struct gdt {
    struct gdt_descriptor entries[11];
    struct tss_descriptor tss;
};

#define NULL_INDEX 0

#define KERNEL_CODE_16_INDEX 1
#define KERNEL_DATA_16_INDEX 2
#define KERNEL_CODE_32_INDEX 3
#define KERNEL_DATA_32_INDEX 4
#define KERNEL_CODE_64_INDEX 5
#define KERNEL_DATA_64_INDEX 6

#define USER_CODE_64_INDEX 9
#define USER_DATA_64_INDEX 10

// This must be in a mutable section
static struct gdt g_gdt = {
    .entries = {
        [NULL_INDEX] = {0},
        [KERNEL_CODE_16_INDEX] = {
            .limit = 0xFFFF,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_KERNEL_CODE | ACC_PRESENT,
            .granularity = 0,
            .base_high8 = 0
        },
        [KERNEL_DATA_16_INDEX] = {
            .limit = 0xFFFF,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_KERNEL_DATA | ACC_PRESENT,
            .granularity = 0,
            .base_high8 = 0
        },
        [KERNEL_CODE_32_INDEX] = {
            .limit = 0xFFFF,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_KERNEL_CODE | ACC_PRESENT,
            .granularity = GRAN_32,
            .base_high8 = 0
        },
        [KERNEL_DATA_32_INDEX] = {
            .limit = 0xFFFF,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_KERNEL_DATA | ACC_PRESENT,
            .granularity = GRAN_32,
            .base_high8 = 0
        },
        [KERNEL_CODE_64_INDEX] = {
            .limit = 0,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_KERNEL_CODE | ACC_PRESENT,
            .granularity = GRAN_64,
            .base_high8 = 0
        },
        [KERNEL_DATA_64_INDEX] = {
            .limit = 0,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_KERNEL_DATA | ACC_PRESENT,
            .granularity = GRAN_64,
            .base_high8 = 0
        },

        // SYSENTER Descriptors
        {0},{0},

        [USER_CODE_64_INDEX] = {
            .limit = 0,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_USER_CODE | ACC_PRESENT,
            .granularity = GRAN_64,
            .base_high8 = 0
        },
        [USER_DATA_64_INDEX] = {
            .limit = 0,
            .base_low16 = 0,
            .base_mid8 = 0,
            .access = ACC_USER_DATA | ACC_PRESENT,
            .granularity = GRAN_64,
            .base_high8 = 0
        },
    },
    .tss = {
        .length = 104, // TODO: Use sizeof(TSS)
        .base_low16 = 0,
        .base_mid8 = 0,
        .flags1 = TSS_64 | TSS_PRESENT,
        .base_high8 = 0,
        .base_upper32 = 0,
        .reserved = 0
    }
};

struct gdt_register {
    uint16_t gdt_size_minus_one;
    struct gdt *gdt;
} __packed;

static struct gdt_register g_gdt_reg = {
    .gdt_size_minus_one = sizeof(g_gdt) - 1,
    .gdt = &g_gdt
};

__optimize(3) uint16_t gdt_get_kernel_code_segment() {
    return KERNEL_CODE_64_INDEX * sizeof(struct gdt_descriptor);
}

__optimize(3) uint16_t gdt_get_user_data_segment() {
    return USER_DATA_64_INDEX * sizeof(struct gdt_descriptor);
}

void gdt_load() {
    asm volatile (
        "lgdt %0\n\t"
        "push $0x28\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "push %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        "mov $0x30, %%eax\n\t"
        "mov %%eax, %%ds\n\t"
        "mov %%eax, %%es\n\t"
        "mov %%eax, %%fs\n\t"
        "mov %%eax, %%gs\n\t"
        "mov %%eax, %%ss\n\t"
        :
        : "m"(g_gdt_reg)
        : "rax", "memory"
    );
}