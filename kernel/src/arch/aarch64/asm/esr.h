/*
 * kernel/src/arch/aarch64/asm/esr.h
 * © suhas pai
 */

#pragma once
#include "lib/macros.h"

enum esr_instr_length_encoding {
    ESR_INSTR_LENGTH_ENCODING_16B,

    /*
     * 32-bit instruction trapped. This value is also used when the exception is
     * one of the following:
     *   An SError interrupt.
     *   An Instruction Abort exception.
     *   A PC alignment fault exception.
     *   An SP alignment fault exception.
     *   A Data Abort exception for which the value of the ISV bit is 0.
     *   An Illegal Execution state exception.
     *   Any debug exception except for Breakpoint instruction exceptions. For
     *      Breakpoint instruction exceptions, this bit has its standard
     *      meaning:
     *        0b0: 16-bit T32 BKPT instruction.
     *        0b1: 32-bit A32 BKPT instruction or A64 BRK instruction.
     *   An exception reported using EC value 0b000000.
     */
    ESR_INSTR_LENGTH_ENCODING_32B,
};

enum esr_error_code {
    ESR_ERROR_CODE_UNKNOWN,
    ESR_ERROR_CODE_TRAPPED_WF,

    // Trapped MCR or MRC access with (coproc==0b1111) that is not reported
    // using EC 0b000000.
    ESR_ERROR_CODE_TRAPPED_MCR_OR_MRC_EC0 = 0b11,

    // Trapped MCRR or MRRC access with (coproc==0b1111) that is not reported
    // using EC 0b000000.
    ESR_ERROR_CODE_TRAPPED_MCRR_OR_MRRC,

    // Trapped MCR or MRC access with (coproc==0b1110).
    ESR_ERROR_CODE_TRAPPED_MCR_OR_MRC,

    ESR_ERROR_CODE_TRAPPED_LDC_OR_SDC = 0b110,
    ESR_ERROR_CODE_TRAPPED_SVE,

    // Trapped execution of an LD64B, ST64B, ST64BV, or ST64BV0 instruction.
    ESR_ERROR_CODE_TRAPPED_LD64B_OR_SD64B = 0b1010,

    // Trapped MRRC access with (coproc==0b1110).
    ESR_ERROR_CODE_TRAPPED_MRRC = 0b1100,
    ESR_ERROR_CODE_BRANCH_TARGET_EXCEPTION,
    ESR_ERROR_CODE_ILLEGAL_EXEC_STATE,
    ESR_ERROR_CODE_SVC_IN_AARCH32 = 0b10001,
    ESR_ERROR_CODE_SVC_IN_AARCH64 = 0b10101,

    /*
     * Trapped MSR, MRS or System instruction execution in AArch64 state, that
     * is not reported using EC 0b000000, 0b000001, or 0b000111.
     *
     * This includes all instructions that cause exceptions that are part of the
     * encoding space defined in 'System instruction class encoding overview',
     * except for those exceptions reported using EC values 0b000000, 0b000001,
     * or 0b000111.
     */
    ESR_ERROR_CODE_TRAPPED_MSR_OR_MRS = 0b11000,
    ESR_ERROR_CODE_TRAPPED_SVE_EC0,

    ESR_ERROR_CODE_TSTART_ACCESS = 0b11011,
    ESR_ERROR_CODE_PTR_AUTH_FAIL = 0b11100,

    ESR_ERROR_CODE_INSTR_ABORT_LOWER_EL = 0b100000,
    ESR_ERROR_CODE_INSTR_ABORT_SAME_EL,
    ESR_ERROR_CODE_PC_ALIGNMENT_FAULT,

    ESR_ERROR_CODE_DATA_ABORT_LOWER_EL = 0b100100,
    ESR_ERROR_CODE_DATA_ABORT_SAME_EL,
    ESR_ERROR_CODE_SP_ALIGNMENT_FAULT,

    ESR_ERROR_CODE_FP_ON_AARCH32_TRAP = 0b101000,
    ESR_ERROR_CODE_FP_ON_AARCH64_TRAP = 0b101100,

    ESR_ERROR_CODE_SERROR_INTERRUPT = 0b101111,

    ESR_ERROR_CODE_BREAKPOINT_LOWER_EL,
    ESR_ERROR_CODE_BREAKPOINT_SAME_EL,

    ESR_ERROR_CODE_SOFTWARE_STEP_LOWER_EL,
    ESR_ERROR_CODE_SOFTWARE_STEP_SAME_EL,

    ESR_ERROR_CODE_WATCHPOINT_LOWER_EL,
    ESR_ERROR_CODE_WATCHPOINT_SAME_EL,

    ESR_ERROR_CODE_BKPT_EXEC_ON_AARCH32 = 0b111000,
    ESR_ERROR_CODE_BKPT_EXEC_ON_AARCH64 = 0b111100,
};

enum esr_serror_aet_kind {
    ESR_SERROR_AET_KIND_UNCONTAINBLE,
    ESR_SERROR_AET_KIND_UNRECOVERABLE,
    ESR_SERROR_AET_KIND_RESTARTABLE,
    ESR_SERROR_AET_KIND_RECOVERABLE,
    ESR_SERROR_AET_KIND_CORRECTED = 6,
};

enum esr_serror_dfsc_kind {
    ESR_SERROR_DFSC_KIND_UNCATEGORIZED,
    ESR_SERROR_DFSC_KIND_ASYNC_ERROR = 0b10001
};

enum esr_serror_shifts {
    ESR_SERROR_EXT_ABORT_SHIFT = 9,
    ESR_SERROR_AET_SHIFT,
    ESR_SERROR_IESB_SHIFT = 13,
    ESR_SERROR_ISS_SHIFT = 23,
    ESR_SERROR_IDS_SHIFT,
};

enum esr_shifts {
    ESR_INSTR_LENGTH_ENCODING_SHIFT,
    ESR_ERROR_CODE_SHIFT,
    ESR_ISS2_SHIFT = 32
};

enum esr_flags {
    __ESR_INSTR_LENGTH_ENCODING = 0b1 << ESR_INSTR_LENGTH_ENCODING_SHIFT,
    __ESR_ERROR_CODE = mask_for_n_bits(6) << ESR_ERROR_CODE_SHIFT,

    __ESR_SERROR_EXT_ABORT = 1 << ESR_SERROR_EXT_ABORT_SHIFT,
    __ESR_SERROR_DFSC = mask_for_n_bits(10),
    __ESR_SERROR_AET = 0b111 << ESR_SERROR_AET_SHIFT,
    __ESR_SERROR_IESB = 1 << ESR_SERROR_IESB_SHIFT,
    __ESR_SERROR_ISS = 1 << ESR_SERROR_ISS_SHIFT,
    __ESR_SERROR_IDS = 1 << ESR_SERROR_IDS_SHIFT,

    // If a memory access generated by an ST64BV or ST64BV0 instruction
    // generates a Data Abort for a Translation fault, Access flag fault, or
    // Permission fault, then this field holds register specifier, Xs.
    __ESR_ISS2 = mask_for_n_bits(5) << ESR_ISS2_SHIFT
};