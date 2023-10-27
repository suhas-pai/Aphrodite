/*
 * kernel/arch/aarch64/asm/sctlr.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

enum sctlr_tag_check_fault {
    SCTLR_TAG_CHECK_FAULT_NO_EFFECT,
    SCTLR_TAG_CHECK_FAULT_SYNC_EXCEPTION,
    SCTLR_TAG_CHECK_FAULT_ASYNC_EXCEPTION,

    // When FEAT_MTE3 is implemented
    SCTLR_TAG_CHECK_FAULT_SYNC_READ_ASYNC_ACCUM_WRITE_EXCEPTION,
};

enum sctlr_shifts {
    SCTLR_TAG_CHECK_FAULT_EL0_SHIFT = 38,
    SCTLR_TAG_CHECK_FAULT_EL1_SHIFT = 40,
    SCTLR_TWE_DELAY_CONFIG_VALUE_SHIFT = 46
};

enum sctlr_flags {
    // MMU enable for EL1&0 stage 1 address translation.
    // EL1&0 stage 1 address translation enabled.
    __SCTLR_MMU = 1ull << 0,

    // Alignment check enable. This is the enable bit for Alignment fault
    // checking at EL1 and EL0.

    // Alignment fault checking enabled when executing at EL1 or EL0.
    // All instructions that load or store one or more registers have an
    // alignment check that the address being accessed is aligned to the size of
    // the data element(s) being accessed. If this check fails it causes an
    // Alignment fault, which is taken as a Data Abort exception.

    __SCTLR_ALIGN_CHECK = 1ull << 1,

    // Stage 1 Cacheability control, for data accesses.
    // This control has no effect on the Stage 1 Cacheability of:
    //   Data access to Normal memory from EL0 and EL1.
    //   Normal memory accesses to the EL1&0 Stage 1 translation tables.

    __SCTLR_CACHE_CNTRL = 1ull << 2,

    // SP Alignment check enable. When set to 1, if a load or store instruction
    // executed at EL1 uses the SP as the base address and the SP is not aligned
    // to a 16-byte boundary, then an SP alignment fault exception is generated.
    // For more information, see 'SP alignment checking'.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on the PE.

    __SCTLR_SP_EL1_ALIGN_CHECK = 1ull << 3,

    // SP Alignment check enable for EL0. When set to 1, if a load or store
    // instruction executed at EL0 uses the SP as the base address and the SP is
    // not aligned to a 16-byte boundary, then an SP alignment fault exception
    // is generated. For more information, see 'SP alignment checking'.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_SP_EL0_ALIGN_CHECK = 1ull << 4,

    // When EL0 is capable of using AArch32:
    // System instruction memory barrier enable. Enables accesses to the DMB,
    // DSB, and ISB System instructions in the (coproc==0b1111) encoding space
    // from EL0:

    // EL0 using AArch32: EL0 execution of the CP15DMB, CP15DSB, and CP15ISB
    // instructions is enabled.

    __SCTLR_CP15BEN = 1ull << 5,

    // When FEAT_LSE2 is implemented:
    // Non-aligned access. This bit controls generation of Alignment faults at
    // EL1 and EL0 under certain conditions.

    // This control bit does not cause LDAPR, LDAPRH, LDAPUR, LDAPURH, LDAPURSH,
    // LDAPURSW, LDAR, LDARH, LDLAR, LDLARH, STLLR, STLLRH, STLR, STLRH, STLUR,
    // or STLURH to generate an Alignment fault if all bytes being accessed are
    // not within a single 16-byte quantity, aligned to 16 bytes.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_UNALIGNED_ACCESS_FAULT = 1ull << 6,

    // When EL0 is capable of using AArch32:
    // IT Disable. Disables some uses of IT instructions at EL0 using AArch32.

    // Any attempt at EL0 using AArch32 to execute any of the following is
    // UNDEFINED and generates an exception, reported using an ESR_ELx.EC value
    // of 0x00, to EL1 or to EL2 when it is implemented and enabled for the
    // current Security state and HCR_EL2.TGE is 1:
    //   All encodings of the IT instruction with hw1[3:0]!=1000.
    //   All encodings of the subsequent instruction with the following values for
    //   hw1:
    //      0b11xxxxxxxxxxxxxx: All 32-bit instructions, and the 16-bit
    //          instructions B, UDF, SVC, LDM, and STM.
    //      0b1011xxxxxxxxxxxx: All instructions in 'Miscellaneous 16-bit
    //          instructions' in the Arm® Architecture Reference Manual, Armv8,
    //          for Armv8-A architecture profile, section F3.2.5.
    //      0b10100xxxxxxxxxxx: ADD Rd, PC, #imm
    //      0b01001xxxxxxxxxxx: LDR Rd, [PC, #imm]
    //      0b0100x1xxx1111xxx: ADD Rdn, PC; CMP Rn, PC; MOV Rd, PC; BX PC;
    //          BLX PC.
    //      0b010001xx1xxxx111: ADD PC, Rm; CMP PC, Rm; MOV PC, Rm. This pattern
    //          also covers unpredictable cases with BLX Rn.
    // These instructions are always UNDEFINED, regardless of whether they would
    // pass or fail the condition code check that applies to them as a result of
    // being in an IT block.
    // It is IMPLEMENTATION DEFINED whether the IT instruction is treated as:
    //  A 16-bit instruction, that can only be followed by another 16-bit
    //      instruction.
    //  The first half of a 32-bit instruction.
    // This means that, for the situations that are UNDEFINED, either the second
    // 16-bit instruction or the 32-bit instruction is UNDEFINED.

    // An implementation might vary dynamically as to whether IT is treated as a
    // 16-bit instruction or the first half of a 32-bit instruction.

    __SCTLR_IT_DISABLE = 1ull << 7,

    // When EL0 is capable of using AArch32:
    // SETEND instruction disable. Disables SETEND instructions at EL0 using
    // AArch32.

    // SETEND instructions are UNDEFINED at EL0 using AArch32 and any attempt at
    // EL0 to access a SETEND instruction generates an exception to EL1, or to
    // EL2 when it is implemented and enabled for the current Security state and
    // HCR_EL2.TGE is 1, reported using an ESR_ELx.EC value of 0x00.

    // If the implementation does not support mixed-endian operation at any
    // Exception level, this bit is Reserved.

    __SCTLR_SETEND_DISABLE = 1ull << 8,

    // User Mask Access. Traps EL0 execution of MSR and MRS instructions that
    // access the PSTATE.{D, A, I, F} masks to EL1, or to EL2 when it is
    // implemented and enabled for the current Security state and HCR_EL2.TGE is
    // 1, from AArch64 state only, reported using an ESR_ELx.EC value of 0x18.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_ALLOW_USER_MASK_ACCESS = 1ull << 9,

    // When FEAT_SPECRES is implemented:
    // Enable EL0 Access to the following instructions:
    //   AArch32 CFPRCTX, DVPRCTX and CPPRCTX instructions.
    //   AArch64 CFP RCTX, DVP RCT and CPP RCTX instructions.

    __SCTLR_ENABLE_EL0_RCTX = 1ull << 10,

    // When FEAT_ExS is implemented:
    //  Exception Exit is Context Synchronizing.

    // An exception return from EL1 is a context synchronizing event.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1,1}, this bit has no effect on execution at EL0.

    __SCTLR_EOS = 1ull << 11,

    // Stage 1 instruction access Cacheability control, for accesses at EL0 and
    // EL1:

    // This control has no effect on the Stage 1 Cacheability of instruction
    // access to Stage 1 Normal memory from EL0 and EL1.

    // If the value of SCTLR_EL1.M is 0, instruction accesses from stage 1 of
    // the EL1&0 translation regime are to Normal, Outer Shareable, Inner
    // Write-Through, Outer Write-Through memory.

    // When the value of the HCR_EL2.DC bit is 1, then instruction access to
    // Normal memory from EL0 and EL1 are Cacheable regardless of the value of
    // the SCTLR_EL1.I bit.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on the PE.

    __SCTLR_STAGE1_INSTR_ACCESS_CACHE_CNTRL = 1ull << 12,

    // When FEAT_PAuth is implemented:
    // Controls enabling of pointer authentication (using the APDBKey_EL1 key)
    // of instruction addresses in the EL1&0 translation regime.

    // For more information, see 'System register control of pointer
    // authentication'.

    // Pointer authentication (using the APDBKey_EL1 key) of data addresses is
    // enabled.

    // This field controls the behavior of the AddPACDB and AuthDB pseudocode
    // functions. Specifically, when the field is 1, AddPACDB returns a copy of
    // a pointer to which a pointer authentication code has been added, and
    // AuthDB returns an authenticated copy of a pointer. When the field is 0,
    // both of these functions are NOP.

    __SCTLR_PAUTH_ENDB = 1ull << 13,

    // Traps EL0 execution of DC ZVA instructions to EL1, or to EL2 when it is
    // implemented and enabled for the current Security state and HCR_EL2.TGE is
    // 1, from AArch64 state only, reported using an ESR_ELx.EC value of 0x18.

    // If FEAT_MTE is implemented, this trap also applies to DC GVA and DC GZVA.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_TRAP_EL0_DCZ_INSTR = 1ull << 14,

    // Traps EL0 accesses to the CTR_EL0 to EL1, or to EL2 when it is
    // implemented and enabled for the current Security state and HCR_EL2.TGE is
    // 1, from AArch64 state only, reported using an ESR_ELx.EC value of 0x18.

    // This control does not cause any instructions to be trapped.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_UCT = 1ull << 15,

    // Traps EL0 execution of WFI instructions to EL1, or to EL2 when it is
    // implemented and enabled for the current Security state and HCR_EL2.TGE
    // is 1, from both Execution states, reported using an ESR_ELx.EC value of
    // 0x01.

    // When FEAT_WFxT is implemented, this trap also applies to the WFIT
    // instruction.

    // In AArch32 state, the attempted execution of a conditional WFI
    // instruction is only trapped if the instruction passes its condition code
    // check.

    // Since a WFE or WFI can complete at any time, even without a Wakeup event,
    // the traps on WFE of WFI are not guaranteed to be taken, even if the WFE
    // or WFI is executed when there is no Wakeup event. The only guarantee is
    // that if the instruction does not complete in finite time in the absence
    // of a Wakeup event, the trap will be taken.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_NTWI = 1ull << 16,

    // Traps EL0 execution of WFE instructions to EL1, or to EL2 when it is
    // implemented and enabled for the current Security state and HCR_EL2.TGE is
    // 1, from both Execution states, reported using an ESR_ELx.EC value of
    // 0x01.

    // When FEAT_WFxT is implemented, this trap also applies to the WFET
    // instruction.

    // In AArch32 state, the attempted execution of a conditional WFE
    // instruction is only trapped if the instruction passes its condition code
    // check.

    // Since a WFE or WFI can complete at any time, even without a Wakeup event,
    // the traps on WFE of WFI are not guaranteed to be taken, even if the WFE
    // or WFI is executed when there is no Wakeup event. The only guarantee is
    // that if the instruction does not complete in finite time in the absence
    // of a Wakeup event, the trap will be taken.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    // Bit 17 is reserved.

    __SCTLR_NTWE = 1ull << 18,

    // Write permission implies XN (Execute-never). For the EL1&0 translation
    // regime, this bit can force all memory regions that are writable to be
    // treated as XN.

    // Any region that is writable in the EL1&0 translation regime is forced to
    // XN for accesses from software executing at EL1 or EL0.

    // This bit applies only when SCTLR_EL1.M bit is set.
    // The WXN bit is permitted to be cached in a TLB.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on the PE.

    __SCTLR_WRITE_IS_XN = 1ull << 19,

    // When FEAT_CSV2_2 is implemented or FEAT_CSV2_1p2 is implemented:
    // Trap EL0 Access to the SCXTNUM_EL0 register, when EL0 is using AArch64.

    // EL0 access to SCXTNUM_EL0 is disabled, causing an exception to EL1, or to
    // EL2 when it is implemented and enabled for the current Security state and
    // HCR_EL2.TGE is 1.

    // The value of SCXTNUM_EL0 is treated as 0.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1,1}, this bit has no effect on execution at EL0.

    __SCTLR_TRAP_EL0_SCXTNUM_EL0 = 1ull << 20,

    // An implicit error synchronization event is added:

    // At each exception taken to EL1.
    // Before the operational pseudocode of each ERET instruction executed at
    // EL1.

    // When the PE is in Debug state, the effect of this field is CONSTRAINED
    // UNPREDICTABLE, and its Effective value might be 0 or 1 regardless of the
    // value of the field. If the Effective value of the field is 1, then an
    // implicit error synchronization event is added after each DCPSx
    // instruction taken to EL1 and before each DRPS instruction executed at
    // EL1, in addition to the other cases where it is added.

    __SCTLR_IMPLICIT_ERR_SYNC_EVENT = 1ull << 21,

    // When FEAT_ExS is implemented:
    // Exception Entry is Context Synchronizing.

    // The taking of an exception to EL1 is a context synchronizing event.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1,1}, this bit has no effect on execution at EL0.

    // If SCTLR_EL1.EIS is set to 0b0:
    //  Indirect writes to ESR_EL1, FAR_EL1, SPSR_EL1, ELR_EL1 are synchronized
    //      on exception entry to EL1, so that a direct read of the register
    //      after exception entry sees the indirectly written value caused by
    //      the exception entry.
    //  Memory transactions, including instruction fetches, from an Exception
    //      level always use the translation resources associated with that
    //      translation regime.
    //  Exception Catch debug events are synchronous debug events.
    //  DCPS* and DRPS instructions are context synchronization events.

    // The following are not affected by the value of SCTLR_EL1.EIS:
    //  Changes to the PSTATE information on entry to EL1.
    //  Behavior of accessing the banked copies of the stack pointer using the
    //      SP register name for loads, stores and data processing instructions.
    //  Exit from Debug state.

    __SCTLR_EIS = 1ull << 22,

    // When FEAT_PAN is implemented:
    //  Set Privileged Access Never, on taking an exception to EL1.

    // The value of PSTATE.PAN is left unchanged on taking an exception to EL1.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_SET_PRIV_ACCESS_NEVER = 1ull << 23,

    // Endianness of data accesses at EL0.
    // If 1, Explicit data accesses at EL0 are big-endian, otherwise,
    // Explicit data accesses at EL0 are little-endian

    // If an implementation only supports Little-endian accesses at EL0, then
    // this bit is RES0. This option is not permitted when SCTLR_EL1.EE is
    // RES1.

    // If an implementation only supports Big-endian accesses at EL0, then this
    // bit is RES1. This option is not permitted when SCTLR_EL1.EE is RES0.

    // This bit has no effect on the endianness of LDTR, LDTRH, LDTRSH, LDTRSW,
    // STTR, and STTRH instructions executed at EL1.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    __SCTLR_SET_EL0_DATA_ACCESS_ENDIAN = 1ull << 24,

    // Endianness of data accesses at EL1, and stage 1 translation table walks
    // in the EL1&0 translation regime.

    // If 1, Explicit data accesses at EL1, and stage 1 translation table walks
    // in the EL1&0 translation regime are big-endian.

    // If an implementation does not provide Big-endian support at Exception
    // levels higher than EL0, this bit is RES0.

    // If an implementation does not provide Little-endian support at Exception
    // levels higher than EL0, this bit is RES1.

    // The EE bit is permitted to be cached in a TLB.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on the PE.

    __SCTLR_SET_EL1_DATA_ACCESS_ENDIAN = 1ull << 25,

    // Traps EL0 execution of cache maintenance instructions, to EL1, or to EL2
    // when it is implemented and enabled for the current Security state and
    // HCR_EL2.TGE is 1, from AArch64 state only, reported using an ESR_ELx.EC
    // value of 0x18.

    // This applies to DC CVAU, DC CIVAC, DC CVAC, DC CVAP, and IC IVAU.
    // If FEAT_DPB2 is implemented, this trap also applies to DC CVADP.

    // If FEAT_MTE is implemented, this trap also applies to DC CIGVAC,
    // DC CIGDVAC, DC CGVAC, DC CGDVAC, DC CGVAP, and DC CGDVAP.

    // If FEAT_DPB2 and FEAT_MTE are implemented, this trap also applies to
    // DC CGVADP and DC CGDVADP.

    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1, 1}, this bit has no effect on execution at EL0.

    // If the Point of Coherency is before any level of data cache, it is
    // IMPLEMENTATION DEFINED whether the execution of any data or unified cache
    // clean, or clean and invalidate instruction that operates by VA to the
    // point of coherency can be trapped when the value of this control is 1.

    // If the Point of Unification is before any level of data cache, it is
    // IMPLEMENTATION DEFINED whether the execution of any data or unified
    // cache clean by VA to the Point of Unification instruction can be trapped
    // when the value of this control is 1.

    // If the Point of Unification is before any level of instruction cache,
    // it is IMPLEMENTATION DEFINED whether the execution of any instruction
    // cache invalidate by VA to the Point of Unification instruction can be
    // trapped when the value of this control is 1.

    __SCTLR_TRAP_USER_CACHE_MAINTAIN_INSTR = 1ull << 26,

    // When FEAT_PAuth is implemented:
    // Controls enabling of pointer authentication (using the APDAKey_EL1 key)
    // of instruction addresses in the EL1&0 translation regime.

    // For more information, see 'System register control of pointer
    // authentication'.

    // Pointer authentication (using the APDAKey_EL1 key) of data addresses is
    // enabled.

    // This field controls the behavior of the AddPACDA and AuthDA pseudocode
    // functions. Specifically, when the field is 1, AddPACDA returns a copy of
    // a pointer to which a pointer authentication code has been added, and
    // AuthDA returns an authenticated copy of a pointer. When the field is 0,
    // both of these functions are NOP

    __SCTLR_PAUTH_ENDA = 1ull << 27,

    // When FEAT_LSMAOC is implemented:
    // No Trap Load Multiple and Store Multiple to
    // Device-nGRE/Device-nGnRE/Device-nGnRnE memory.

    // All memory accesses by A32 and T32 Load Multiple and Store Multiple at
    // EL0 that are marked at stage 1 as Device-nGRE/Device-nGnRE/Device-nGnRnE
    // memory are not trapped.

    // This bit is permitted to be cached in a TLB.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1,1}, this bit has no effect on execution at EL0.

    __SCTLR_NO_TRAP_LMSMD = 1ull << 28,

    // Load Multiple and Store Multiple Atomicity and Ordering Enable.

    // The ordering and interrupt behavior of A32 and T32 Load Multiple and
    // Store Multiple at EL0 is as defined for Armv8.0.

    // This bit is permitted to be cached in a TLB.
    // When FEAT_VHE is implemented, and the value of HCR_EL2.{E2H, TGE} is
    // {1,1}, this bit has no effect on execution at EL0.

    __SCTLR_LMSM_ATOMICITY_ORDERING_ENABLE = 1ull << 29,

    // Controls enabling of pointer authentication (using the APIBKey_EL1 key)
    // of instruction addresses in the EL1&0 translation regime.

    // For more information, see 'System register control of pointer
    // authentication'.

    // Pointer authentication (using the APIBKey_EL1 key) of instruction
    // addresses is enabled.

    // This field controls the behavior of the AddPACIB and AuthIB pseudocode
    // functions. Specifically, when the field is 1, AddPACIB returns a copy of
    // a pointer to which a pointer authentication code has been added, and
    // AuthIB returns an authenticated copy of a pointer. When the field is 0,
    // both of these functions are NOP.

    __SCTLR_PAUTH_ENIB = 1ull << 30,

    // When FEAT_PAuth is implemented:
    // Controls enabling of pointer authentication (using the APIAKey_EL1 key)
    // of instruction addresses in the EL1&0 translation regime.

    // For more information, see 'System register control of pointer
    // authentication'.

    // Pointer authentication (using the APIAKey_EL1 key) of instruction
    // addresses is enabled.

    // This field controls the behavior of the AddPACIA and AuthIA pseudocode
    // functions. Specifically, when the field is 1, AddPACIA returns a copy of
    // a pointer to which a pointer authentication code has been added, and
    // AuthIA returns an authenticated copy of a pointer. When the field is 0,
    // both of these functions are NOP.

    __SCTLR_PAUTH_ENIA = 1ull << 31,

    // When FEAT_CMOW is implemented:
    // Controls cache maintenance instruction permission for the following
    // instructions executed at EL0.
    //   IC IVAU, DC CIVAC, DC CIGDVAC and DC CIGVAC.

    // If enabled as a result of SCTLR_EL1.UCI==1, these instructions executed
    // at EL0 with stage 1 read permission, but without stage 1 write
    // permission, generate a stage 1 permission fault.

    // When AArch64.HCR_EL2.{E2H, TGE} is {1, 1}, this bit has no effect on
    // execution at EL0.

    // For this control, stage 1 has write permission if all of the following apply:
    //  AP[2] is 0 or DBM is 1 in the stage 1 descriptor.
    //  Where APTable is in use, APTable[1] is 0 for all levels of the
    //      translation table.

    // This bit is permitted to be cached in a TLB.

    __SCTLR_CMOW_EL0 = 1ull << 32,

    // When FEAT_MOPS is implemented and (HCR_EL2.E2H == 0 or HCR_EL2.TGE == 0):
    // Memory Copy and Memory Set instructions Enable. Enables execution of the
    // Memory Copy and Memory Set instructions at EL0.

    // This control does not cause any instructions to be UNDEFINED.
    // When FEAT_MOPS is implemented and HCR_EL2.{E2H, TGE} is {1, 1}, the
    // Effective value of this bit is 0b1.

    __SCTLR_MEMCPY_MEMSET_EL0_ENABLE = 1ull << 33,

    // When FEAT_BTI is implemented:
    // PAC Branch Type compatibility at EL0.

    // When the PE is executing at EL0, PACIASP and PACIBSP are not compatible
    // with PSTATE.BTYPE == 0b11.

    // When the value of HCR_EL2.{E2H, TGE} is {1, 1}, the value of
    // SCTLR_EL1.BT0 has no effect on execution at EL0

    __SCTLR_BT_EL0 = 1ull << 35,

    // When FEAT_BTI is implemented:
    // PAC Branch Type compatibility at EL1.

    // When the PE is executing at EL1, PACIASP and PACIBSP are not compatible
    // with PSTATE.BTYPE == 0b11.

    __SCTLR_BT_EL1 = 1ull << 36,

    // When FEAT_MTE2 is implemented:
    // When synchronous exceptions are not being generated by Tag Check Faults,
    // this field controls whether on exception entry into EL1, all Tag Check
    // Faults due to instructions executed before exception entry, that are
    // reported asynchronously, are synchronized into TFSRE0_EL1 and TFSR_EL1
    // registers.

    // Tag Check Faults are synchronized on entry to EL1.
    __SCTLR_ITFSB = 1ull << 37,

    // Tag Check Fault in EL0. When HCR_EL2.{E2H,TGE} != {1,1}, controls th
    //  effect of Tag Check Faults due to Loads and Stores in EL0.
    // If FEAT_MTE3 is not implemented, the value 0b11 is reserved.

    // Software may change this control bit on a context switch.

    __SCTLR_EL0_TAG_CHECK_FAULT = 0b11ull << SCTLR_TAG_CHECK_FAULT_EL0_SHIFT,

    // Tag Check Fault in EL1. Controls the effect of Tag Check Faults due to
    // Loads and Stores in EL1.
    // If FEAT_MTE3 is not implemented, the value 0b11 is reserved.

    __SCTLR_EL1_TAG_CHECK_FAULT = 0b11ull << SCTLR_TAG_CHECK_FAULT_EL1_SHIFT,

    // When FEAT_MTE2 is implemented:
    // Allocation Tag Access in EL0. When SCR_EL3.ATA=1, HCR_EL2.ATA=1, and
    // HCR_EL2.{E2H, TGE} != {1, 1}, controls EL0 access to Allocation Tags.

    __SCTLR_ALLOW_EL0_ALLOC_TAG_ACCESS = 1ull << 42,

    // When FEAT_MTE2 is implemented:
    // Allocation Tag Access in EL1. When SCR_EL3.ATA=1 and HCR_EL2.ATA=1,
    // controls EL1 access to Allocation Tags.

    __SCTLR_ALLOW_EL1_ALLOC_TAG_ACCESS = 1ull << 43,

    // When FEAT_SSBS is implemented:
    // Default PSTATE.SSBS value on Exception Entry.

    __SCTLR_DEFAULT_SSBS_VALUE = 1ull << 44,

    // When FEAT_TWED is implemented:
    // TWE Delay Enable. Enables a configurable delayed trap of the WFE*
    // instruction caused by SCTLR_EL1.nTWE.

    // The delay for taking the trap is at least the number of cycles defined in
    // SCTLR_EL1.TWEDEL.

    __SCTLR_TWE_DELAY_CONFIG_ENABLE = 1ull << 45,

    // TWE Delay. A 4-bit unsigned number that, when SCTLR_EL1.TWEDEn is 1,
    // encodes the minimum delay in taking a trap of WFE* caused by
    // SCTLR_EL1.nTWE as 2(TWEDEL + 8) cycles

    __SCTLR_TWE_DELAY_CONFIG_VALUE =
        0b1111ull << SCTLR_TWE_DELAY_CONFIG_VALUE_SHIFT,

    // When FEAT_LS64_V is implemented:
    // When HCR_EL2.{E2H, TGE} != {1, 1}, traps execution of an ST64BV
    // instruction at EL0 to EL1.

    // This control does not cause any instructions to be trapped.

    // A trap of an ST64BV instruction is reported using an ESR_ELx.EC value of
    // 0x0A, with an ISS code of 0x0000000.

    __SCTLR_DONT_TRAP_EL1_ST64BV = 1ull << 54,

    // When FEAT_LS64_ACCDATA is implemented:
    // When HCR_EL2.{E2H, TGE} != {1, 1}, traps execution of an ST64BV0
    // instruction at EL0 to EL1.

    // This control does not cause any instructions to be trapped.

    // A trap of an ST64BV0 instruction is reported using an ESR_ELx.EC value of
    // 0x0A, with an ISS code of 0x0000001.

    __SCTLR_DONT_TRAP_EL1_ST64BV0 = 1ull << 55,

    // When FEAT_LS64 is implemented:
    // When HCR_EL2.{E2H, TGE} != {1, 1}, traps execution of an LD64B or ST64B
    // instruction at EL0 to EL1.

    // This control does not cause any instructions to be trapped.
    // A trap of an LD64B or ST64B instruction is reported using an ESR_ELx.EC
    // value of 0x0A, with an ISS code of 0x0000002.

    __SCTLR_DONT_TRAP_EL1_LD64B_ST64B = 1ull << 56,

    // When FEAT_PAN3 is implemented:
    // Enhanced Privileged Access Never. When PSTATE.PAN is 1, determines
    // whether an EL1 data access to a page with stage 1 EL0 instruction access
    // permission generates a Permission fault as a result of the Privileged
    // Access Never mechanism.

    // An EL1 data access to a page with stage 1 EL0 data access permission or
    // stage 1 EL0 instruction access permission generates a Permission fault.
    // Any speculative data accesses that would generate a Permission fault if
    // the accesses were not speculative will not cause an allocation into a
    // cache.

    // This bit is permitted to be cached in a TLB.
    __SCTLR_ENHANCED_PAN = 1ull << 57,

    // When FEAT_NMI is implemented:
    // Non-maskable Interrupt enable.

    // This control enables all of the following:
    //  The use of the PSTATE.ALLINT interrupt mask.
    //  IRQ and FIQ interrupts to have Superpriority as an additional attribute.
    //  PSTATE.SP to be used as an interrupt mask.

    __SCTLR_NMI_ENABLE = 1ull << 61,

    // When FEAT_NMI is implemented:
    // SP Interrupt Mask enable. When SCTLR_EL1.NMI is 1, controls whether
    // PSTATE.SP acts as an interrupt mask, and controls the value of
    // PSTATE.ALLINT on taking an exception to EL1.

    // When PSTATE.SP is 1 and execution is at EL1, an IRQ or FIQ interrupt that
    // is targeted to EL1 is masked regardless of any denotion of Superpriority.
    // PSTATE.ALLINT is set to 0 on taking an exception to EL1.

    __SCTLR_SP_INTR_MASK_ENABLE = 1ull << 62,

    // When FEAT_TIDCP1 is implemented:
    // Trap IMPLEMENTATION DEFINED functionality. When HCR_EL2.{E2H, TGE} !=
    // {1, 1}, traps EL0 accesses to the encodings reserved for IMPLEMENTATION
    // DEFINED functionality to EL1.

    // Instructions accessing the following System register or System
    // instruction spaces are trapped to EL1 by this mechanism:
    //  In AArch64 state, EL0 access to the encodings in the following reserved
    //  encoding spaces are trapped and reported using EC syndrome 0x18:
    //      IMPLEMENTATION DEFINED System instructions, which are accessed using
    //          SYS and SYSL, with CRn == {11, 15}.
    //      IMPLEMENTATION DEFINED System registers, which are accessed using
    //          MRS and MSR with the S3_<op1>_<Cn>_<Cm>_<op2> register name.
    // In AArch32 state, EL0 MCR and MRC access to the following encodings are
    // trapped and reported using EC syndrome 0x03:
    //  All coproc==p15, CRn==c9, opc1 == {0-7}, CRm == {c0-c2, c5-c8},
    //      opc2 == {0-7}.
    //  All coproc==p15, CRn==c10, opc1 =={0-7}, CRm == {c0, c1, c4, c8},
    //      opc2 == {0-7}.
    //  All coproc==p15, CRn==c11, opc1=={0-7}, CRm == {c0-c8, c15},
    //      opc2 == {0-7}.

    __SCTLR_TIDCP = 1ull << 63,
};


__optimize(3) static inline uint64_t read_sctlr_el0() {
    uint64_t result = 0;
    asm volatile("mrs %0, sctlr_el0" : "=r"(result));

    return result;
}

__optimize(3) static inline uint64_t read_sctlr_el1() {
    uint64_t result = 0;
    asm volatile("mrs %0, sctlr_el1" : "=r"(result));

    return result;
}

__optimize(3) static inline void write_sctlr_el0(const uint64_t value) {
    asm volatile("msr sctlr_el0, %0" :: "r"(value));
}

__optimize(3) static inline void write_sctlr_el1(const uint64_t value) {
    asm volatile("msr sctlr_el1, %0" :: "r"(value));
}