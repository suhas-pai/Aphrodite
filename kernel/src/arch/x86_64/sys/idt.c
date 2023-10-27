/*
 * kernel/arch/x86_64/sys/idt.c
 * © suhas pai
 */

#include "asm/cr.h"
#include "asm/stack_trace.h"

#include "cpu/isr.h"
#include "cpu/util.h"

#include "dev/printk.h"

#include "gdt.h"
#include "pic.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_hi;
    uint32_t reserved;
};

struct idt_register {
    uint16_t idt_size_minus_one;
    struct idt_entry *idt;
} __packed;

enum exception {
    EXCEPTION_DIVIDE_BY_ZERO,
    EXCEPTION_DEBUG,

    // NMI = Non-Maskable Interrupt
    EXCEPTION_NMI,

    // Breakpoint = Debug Trap
    EXCEPTION_BREAKPOINT,

    // Overflow = Arithmetic Overflow
    EXCEPTION_OVERFLOW,

    // Bound = Array Bounds Exceeded. For BOUND instruction
    EXCEPTION_BOUND,

    EXCEPTION_INVALID_OPCODE,
    EXCEPTION_DEVICE_NOT_AVAILABLE,
    EXCEPTION_DOUBLE_FAULT,

    EXCEPTION_COPROC_SEGMENT_OVERRUN,
    EXCEPTION_INVALID_TSS,

    EXCEPTION_SEGMENT_NOT_PRESENT,
    EXCEPTION_STACK_FAULT,
    EXCEPTION_GENERAL_PROTECTION_FAULT,
    EXCEPTION_PAGE_FAULT,

    // 15 = Reserved

    EXCEPTION_FPU_FAULT = 16,
    EXCEPTION_ALIGNMENT_CHECK,
    EXCEPTION_MACHINE_CHECK,
    EXCEPTION_SIMD_FLOATING_POINT,
    EXCEPTION_VIRTUALIZATION_EXCEPTION,
    EXCEPTION_CONTROL_PROTECTION_EXCEPTION,

    // 22-27 = Reserved
    EXCEPTION_HYPERVISOR_EXCEPTION = 28,
    EXCEPTION_VMM_EXCEPTION,
    EXCEPTION_SECURITY_EXCEPTION,
};

static struct idt_entry g_idt[256] = {0};
extern void *const idt_thunks[];

void
idt_set_vector(const idt_vector_t vector,
               const uint8_t ist,
               const uint8_t flags)
{
    const uint64_t handler_addr = (uint64_t)idt_thunks[vector];
    g_idt[vector] = (struct idt_entry){
        .offset_low = (uint16_t)handler_addr,
        .selector = gdt_get_kernel_code_segment(),
        .ist = ist,
        .flags = flags,
        .offset_mid = (uint16_t)(handler_addr >> 16),
        .offset_hi = (uint32_t)(handler_addr >> 32),
        .reserved = 0
    };
}

void idt_load() {
    static struct idt_register idt_reg = {
        .idt_size_minus_one = sizeof(g_idt) - 1,
        .idt = g_idt
    };

    asm volatile ("lidt %0" :: "m"(idt_reg) : "memory");
}

void idt_init() {
    pic_remap(247, 255);
    for (uint16_t i = 0; i != countof(g_idt); i++) {
        idt_set_vector(i, IST_NONE, /*flags=*/0x8e);
    }

    idt_load();
}

static
void handle_exception(const uint64_t int_no, irq_context_t *const context) {
    switch ((enum exception)int_no) {
        case EXCEPTION_DIVIDE_BY_ZERO:
            printk(LOGLEVEL_ERROR, "Divide by zero exception\n");
            break;
        case EXCEPTION_DEBUG:
            printk(LOGLEVEL_ERROR, "Debug exception\n");
            cpu_halt();
        case EXCEPTION_NMI:
            printk(LOGLEVEL_ERROR, "NMI exception\n");
            break;
        case EXCEPTION_BREAKPOINT:
            printk(LOGLEVEL_ERROR, "Breakpoint exception\n");
            break;
        case EXCEPTION_OVERFLOW:
            printk(LOGLEVEL_ERROR, "Overflow exception\n");
            break;
        case EXCEPTION_BOUND:
            printk(LOGLEVEL_ERROR, "BOUND exception\n");
            break;
        case EXCEPTION_INVALID_OPCODE:
            printk(LOGLEVEL_ERROR, "Invalid opcode exception\n");
            break;
        case EXCEPTION_DEVICE_NOT_AVAILABLE:
            printk(LOGLEVEL_ERROR, "Device not available exception\n");
            break;
        case EXCEPTION_DOUBLE_FAULT:
            printk(LOGLEVEL_ERROR, "Double fault exception\n");
            break;
        case EXCEPTION_COPROC_SEGMENT_OVERRUN:
            printk(LOGLEVEL_ERROR, "Coprocessor segment overrun exception\n");
            break;
        case EXCEPTION_INVALID_TSS:
            printk(LOGLEVEL_ERROR, "Invalid TSS exception\n");
            break;
        case EXCEPTION_SEGMENT_NOT_PRESENT:
            printk(LOGLEVEL_ERROR, "Segment not present exception\n");
            break;
        case EXCEPTION_STACK_FAULT:
            printk(LOGLEVEL_ERROR, "Stack fault exception\n");
            break;
        case EXCEPTION_GENERAL_PROTECTION_FAULT:
            printk(LOGLEVEL_ERROR, "General protection fault exception\n");
            break;
        case EXCEPTION_PAGE_FAULT:
            printk(LOGLEVEL_ERROR,
                   "Page Fault accessing %p from %p\n",
                   (void *)read_cr2(),
                   (void *)context->rip);

            print_stack_trace(/*max_lines=*/10);
            break;
        case EXCEPTION_FPU_FAULT:
            printk(LOGLEVEL_ERROR, "FPU fault exception\n");
            break;
        case EXCEPTION_ALIGNMENT_CHECK:
            printk(LOGLEVEL_ERROR, "Alignment check exception\n");
            break;
        case EXCEPTION_MACHINE_CHECK:
            printk(LOGLEVEL_ERROR, "Machine check exception\n");
            break;
        case EXCEPTION_SIMD_FLOATING_POINT:
            printk(LOGLEVEL_ERROR, "SIMD floating point exception\n");
            break;
        case EXCEPTION_VIRTUALIZATION_EXCEPTION:
            printk(LOGLEVEL_ERROR, "Virtualization exception\n");
            break;
        case EXCEPTION_CONTROL_PROTECTION_EXCEPTION:
            printk(LOGLEVEL_ERROR, "Control protection exception\n");
            break;
        case EXCEPTION_HYPERVISOR_EXCEPTION:
            printk(LOGLEVEL_ERROR, "Hypervisor exception\n");
            break;
        case EXCEPTION_VMM_EXCEPTION:
            printk(LOGLEVEL_ERROR, "VMM exception\n");
            break;
        case EXCEPTION_SECURITY_EXCEPTION:
            printk(LOGLEVEL_ERROR, "Security exception\n");
            break;
    }

    cpu_halt();
}

void idt_register_exception_handlers() {
    for (idt_vector_t vector = 0; vector != 0x20; vector++) {
        isr_set_vector(vector, handle_exception, &ARCH_ISR_INFO_NONE());
    }
}