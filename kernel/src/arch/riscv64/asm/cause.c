/*
 * kernel/src/arch/riscv64/asm/cause.c
 * © suhas pai
 */

#include "lib/assert.h"
#include "cause.h"

__optimize(3) struct string_view
cause_exception_kind_get_sv(const enum cause_exception_kind kind) {
    switch (kind) {
        case CAUSE_EXCEPTION_NONE:
            verify_not_reached();
        case CAUSE_EXCEPTION_INST_ADDR_MI:
            return SV_STATIC("misaligned instruction address");
        case CAUSE_EXCEPTION_INST_ACCESS_FAULT:
            return SV_STATIC("instruction access fault");
        case CAUSE_EXCEPTION_ILLEGAL_INST:
            return SV_STATIC("illegal instruction");
        case CAUSE_EXCEPTION_BREAKPOINT:
            return SV_STATIC("breakpoint");
        case CAUSE_EXCEPTION_LOAD_ADDR_MIS:
            return SV_STATIC("load address misaligned");
        case CAUSE_EXCEPTION_LOAD_ACCESS_FAULT:
            return SV_STATIC("load access fault");
        case CAUSE_EXCEPTION_STORE_AMO_ADDR_MIS:
            return SV_STATIC("store amo address misaligned");
        case CAUSE_EXCEPTION_STORE_AMO_ACCESS_FAULT:
            return SV_STATIC("store amo access fault");
        case CAUSE_EXCEPTION_U_ECALL:
            return SV_STATIC("user attempted to ecall");
        case CAUSE_EXCEPTION_S_ECALL:
            return SV_STATIC("supervisor attempted to ecall");
        case CAUSE_EXCEPTION_VS_ECALL:
            return SV_STATIC("virtual-supervisor attempted to ecall");
        case CAUSE_EXCEPTION_M_ECALL:
            return SV_STATIC("machine attempted to ecall");
        case CAUSE_EXCEPTION_INST_PAGE_FAULT:
            return SV_STATIC("instruction page-fault");
        case CAUSE_EXCEPTION_LOAD_PAGE_FAULT:
            return SV_STATIC("load page-fault");
        case CAUSE_EXCEPTION_STORE_PAGE_FAULT:
            return SV_STATIC("store page-fault");
        case CAUSE_EXCEPTION_SEMIHOST:
            return SV_STATIC("semi-host");
        case CAUSE_EXCEPTION_INST_GUEST_PAGE_FAULT:
            return SV_STATIC("instruction guest page fault");
        case CAUSE_EXCEPTION_LOAD_GUEST_ACCESS_FAULT:
            return SV_STATIC("load guest page fault");
        case CAUSE_EXCEPTION_VIRT_INSTRUCTION_FAULT:
            return SV_STATIC("virt-instruction guest page fault");
        case CAUSE_EXCEPTION_STORE_GUEST_AMO_ACCESS_FAULT:
            return SV_STATIC("store guest amo access fault");
    }

    verify_not_reached();
}