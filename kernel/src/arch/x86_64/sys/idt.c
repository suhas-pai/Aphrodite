/*
 * kernel/src/arch/x86_64/sys/idt.c
 * © suhas pai
 */

#include "apic/lapic.h"

#include "asm/cr.h"
#include "asm/error_code.h"
#include "asm/irqs.h"
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

void
handle_exception(const uint64_t intr_no, struct thread_context *const context) {
    this_cpu_mut()->in_exception = true;

    const char *except_str = "<unknown>";
    switch ((enum exception)intr_no) {
        case EXCEPTION_DIVIDE_BY_ZERO:
            except_str = "Divide by zero exception";
            break;
        case EXCEPTION_DEBUG:
            except_str = "Debug exception";
            cpu_idle();
        case EXCEPTION_NMI:
            except_str = "NMI exception";
            break;
        case EXCEPTION_BREAKPOINT:
            except_str = "Breakpoint exception";
            break;
        case EXCEPTION_OVERFLOW:
            except_str = "Overflow exception";
            break;
        case EXCEPTION_BOUND:
            except_str = "BOUND exception";
            break;
        case EXCEPTION_INVALID_OPCODE:
            except_str = "Invalid opcode exception";
            break;
        case EXCEPTION_DEVICE_NOT_AVAILABLE:
            except_str = "Device not available exception";
            break;
        case EXCEPTION_DOUBLE_FAULT:
            except_str = "Double fault exception";
            break;
        case EXCEPTION_COPROC_SEGMENT_OVERRUN:
            except_str = "Coprocessor segment overrun exception";
            break;
        case EXCEPTION_INVALID_TSS:
            except_str = "Invalid TSS exception";
            break;
        case EXCEPTION_SEGMENT_NOT_PRESENT:
            except_str = "Segment not present exception";
            break;
        case EXCEPTION_STACK_FAULT:
            except_str = "Stack fault exception";
            break;
        case EXCEPTION_GENERAL_PROTECTION_FAULT:
            except_str = "General protection fault exception";
            break;
        case EXCEPTION_PAGE_FAULT:
            printk(LOGLEVEL_ERROR,
                   "Page Fault accessing %p from instruction at %p\n"
                   "\tPresent: %s\n"
                   "\tWrite: %s\n"
                   "\tUser: %s\n"
                   "\tReserved Write: %s\n"
                   "\tInstruction Fetch: %s\n"
                   "\tProtection Key: %s\n"
                   "\tShadow Stack: %s\n"
                   "\tSoftware Guard Extensions: %s\n",
                   (void *)read_cr2(),
                   (void *)context->rip,
                   context->err_code & __PAGE_FAULT_ERROR_CODE_PRESENT
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_WRITE
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_USER
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_RESERVED_WRITE
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_INSTRUCTION_FETCH
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_PROTECTION_KEY
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_SHADOW_STACK
                    ? "yes" : "no",
                   context->err_code & __PAGE_FAULT_ERROR_CODE_SW_GUARD_EXT
                    ? "yes" : "no");

            printk(LOGLEVEL_ERROR, "Stack Frame:\n");
            print_stack_trace(/*max_lines=*/10);

            cpu_halt();
        case EXCEPTION_FPU_FAULT:
            except_str = "FPU fault exception";
            break;
        case EXCEPTION_ALIGNMENT_CHECK:
            except_str = "Alignment check exception";
            break;
        case EXCEPTION_MACHINE_CHECK:
            except_str = "Machine check exception";
            break;
        case EXCEPTION_SIMD_FLOATING_POINT:
            except_str = "SIMD floating point exception";
            break;
        case EXCEPTION_VIRTUALIZATION_EXCEPTION:
            except_str = "Virtualization exception";
            break;
        case EXCEPTION_CONTROL_PROTECTION_EXCEPTION:
            except_str = "Control protection exception";
            break;
        case EXCEPTION_HYPERVISOR_EXCEPTION:
            except_str = "Hypervisor exception";
            break;
        case EXCEPTION_VMM_EXCEPTION:
            except_str = "VMM exception";
            break;
        case EXCEPTION_SECURITY_EXCEPTION:
            except_str = "Security exception";
            break;
    }

    printk(LOGLEVEL_WARN,
           "Exception (%s) at %p\n",
           except_str,
           (void *)context->rip);

    print_stack_trace(/*max_lines=*/10);

    lapic_eoi();
    cpu_halt();
}

void idt_register_exception_handlers() {
    for (idt_vector_t vector = 0; vector != 0x20; vector++) {
        isr_set_vector(vector, handle_exception, &ARCH_ISR_INFO_NONE());
    }
}