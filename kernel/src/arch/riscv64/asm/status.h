/*
 * kernel/src/arch/riscv64/asm/status.h
 * © suhas pai
 */

#pragma once

#include "lib/macros.h"
#include "csr.h"

enum mstatus_flags {
    __MSTATUS_USER_INT_ENABLE = 1ull << 0,
    __MSTATUS_SUPERVISOR_INT_ENABLE = 1ull << 1,
    __MSTATUS_MACHINE_INT_ENABLE = 1ull << 3,
    __MSTATUS_USER_PRIV_INT_ENABLE = 1ull << 4,
    __MSTATUS_SUPERVISOR_PRIV_INT_ENABLE = 1ull << 5,
    __MSTATUS_MACHINE_PRIV_INT_ENABLE = 1ull << 7,
    __MSTATUS_SUPERVISOR_PP = 1ull << 8,
    __MSTATUS_MACHINE_PP = 0b11ull << 11,
    __MSTATUS_FP_STATUS = 0b11ull << 13,
    __MSTATUS_USER_EXT_STATUS = 0b11ull << 15,
    __MSTATUS_MODIFY_PRIVILEGE = 1ull << 17,
    __MSTATUS_ALLOW_SUPERVISOR_ACCESS_USER_MEM = 1ull << 18,
    __MSTATUS_ALLOW_EXEC_ON_EXEC_ONLY_PAGES = 1ull << 19,
    __MSTATUS_TRAP_VIRT_MEM_MGMT_INSTR = 1ull << 20,
    __MSTATUS_TRAP_WAIT_INSTR = 1ull << 21,
    __MSTATUS_FS_OR_XS_IS_DIRTY = 1ull << 31
};

enum sstatus_flags {
    __SSTATUS_USER_INT_ENABLE = 1ull << 0,
    __SSTATUS_SUPERVISOR_INT_ENABLE = 1ull << 1,
    __SSTATUS_USER_PRIV_INT_ENABLE = 1ull << 4,
    __SSTATUS_SUPERVISOR_PRIV_INT_ENABLE = 1ull << 5,
    __SSTATUS_SUPERVISOR_PP = 1ull << 8,
    __SSTATUS_FP_STATUS = 0b11ull << 13,
    __SSTATUS_USER_EXT_STATUS = 0b11ull << 15,
    __SSTATUS_ALLOW_SUPERVISOR_ACCESS_USER_MEM = 1ull << 18,
    __SSTATUS_ALLOW_EXEC_ON_EXEC_ONLY_PAGES = 1ull << 19,
    __SSTATUS_FS_OR_XS_IS_DIRTY = 1ull << 31
};

__optimize(3) static inline uint64_t read_sstatus() {
    return csr_read(sstatus);
}

__optimize(3) static inline uint64_t write_sstatus(const uint64_t value) {
    return csr_write(sstatus, value);
}