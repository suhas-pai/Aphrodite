/*
 * kernel/arch/aarch64/cpu.c
 * © suhas pai
 */

#include "asm/id_regs.h"
#include "asm/tcr.h"

#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "cpu.h"
#include "features.h"

static struct cpu_info g_base_cpu_info = {
    .pagemap = &kernel_pagemap,
    .pagemap_node = LIST_INIT(g_base_cpu_info.pagemap_node),

    .cpu_list = LIST_INIT(g_base_cpu_info.cpu_list),
    .spur_int_count = 0,

    .cpu_interface_number = 0,
    .acpi_processor_id = 0,

    .spe_overflow_interrupt = 0,
    .mpidr = 0,
};

static struct cpu_features g_cpu_features = {0};
struct list g_cpu_list = LIST_INIT(g_cpu_list);
static bool g_base_cpu_init = false;

__optimize(3) const struct cpu_info *get_base_cpu_info() {
    assert(__builtin_expect(g_base_cpu_init, 1));
    return &g_base_cpu_info;
}

__optimize(3) const struct cpu_info *get_cpu_info() {
    return get_cpu_info_mut();
}

__optimize(3) struct cpu_info *get_cpu_info_mut() {
    assert(__builtin_expect(g_base_cpu_init, 1));

    struct cpu_info *result = NULL;
    asm volatile ("mrs %0, tpidr_el1" : "=r"(result));

    return result;
}

__optimize(3) const struct cpu_features *cpu_get_features() {
    return &g_cpu_features;
}

void collect_cpu_features() {
    const uint64_t id_aa64isar0 = read_id_aa64isar0_el1();
    const uint64_t id_aa64isar1 = read_id_aa64isar1_el1();
    const uint64_t id_aa64isar2 = read_id_aa64isar2_el1();
    const uint64_t id_aa64pfr0 = read_id_aa64pfr0_el1();
    const uint64_t id_aa64pfr1 = read_id_aa64pfr1_el1();
    const uint64_t id_aa64mmfr0 = read_id_aa64mmfr0_el1();
    const uint64_t id_aa64mmfr1 = read_id_aa64mmfr1_el1();
    const uint64_t id_aa64mmfr2 = read_id_aa64mmfr2_el1();
    //const uint64_t id_aa64mmfr3 = read_id_aa64mmfr3_el1();
    //const uint64_t id_aa64smfr0 = read_id_aa64smfr0_el1();
    //const uint64_t id_aa64zfr0 = read_id_aa64zfr0_el1();
    const uint64_t id_aa64dfr0 = read_id_aa64dfr0_el1();
    const uint64_t id_aa64dfr1 = read_id_aa64dfr1_el1();
    const uint64_t tcr_el1 = read_tcr_el1();

    const enum id_aa64isar0_el1_aes_support aa64isar0_el1_aes_support =
        (id_aa64isar0 & __ID_AA64ISAR0_EL1_AES) >>
            ID_AA64ISAR0_EL1_AES_SUPPORT_SHIFT;

    switch (aa64isar0_el1_aes_support) {
        case ID_AA64ISAR0_EL1_AES_SUPPORT_NONE:
            break;
        case ID_AA64ISAR0_EL1_AES_SUPPORT_ONLY_AES:
            g_cpu_features.aes = CPU_FEAT_AES;
            break;
        case ID_AA64ISAR0_EL1_AES_SUPPORT_AES_AND_PMULL:
            g_cpu_features.aes = CPU_FEAT_AES_PMULL;
            break;
    }

    g_cpu_features.sha1 = (id_aa64isar0 & __ID_AA64ISAR0_EL1_SHA1) != 0;
    const enum id_aa64isar0_el1_sha2_support aa64isar0_el1_sha2_support =
        (id_aa64isar0 & __ID_AA64ISAR0_EL1_SHA2) >>
            ID_AA64ISAR0_EL1_SHA2_SUPPORT_SHIFT;

    switch (aa64isar0_el1_sha2_support) {
        case ID_AA64ISAR0_EL1_SHA2_SUPPORT_NONE:
            break;
        case ID_AA64ISAR0_EL1_SHA2_SUPPORT_SHA256:
            g_cpu_features.sha2 = CPU_FEAT_SHA256;
            break;
        case ID_AA64ISAR0_EL1_SHA2_SUPPORT_SHA512:
            g_cpu_features.sha2 = CPU_FEAT_SHA512;
            break;
    }

    g_cpu_features.crc32 = (id_aa64isar0 & __ID_AA64ISAR0_EL1_CRC32) != 0;
    const enum id_aa64isar0_el1_atomic_support aa64isar0_el1_atomic_support =
        (id_aa64isar0 & __ID_AA64ISAR0_EL1_ATOMIC) >>
            ID_AA64ISAR0_EL1_ATOMIC_SUPPORT_SHIFT;

    switch (aa64isar0_el1_atomic_support) {
        case ID_AA64ISAR0_EL1_ATOMIC_SUPPORT_NONE:
            break;
        case ID_AA64ISAR0_EL1_ATOMIC_SUPPORT_LSE:
            g_cpu_features.atomic = CPU_FEAT_ATOMIC_LSE;
            break;
        case ID_AA64ISAR0_EL1_ATOMIC_SUPPORT_LSE128:
            g_cpu_features.atomic = CPU_FEAT_ATOMIC_LSE128;
            break;
    }

    g_cpu_features.tme = (id_aa64isar0 & __ID_AA64ISAR0_EL1_TME) != 0;
    g_cpu_features.rdm = (id_aa64isar0 & __ID_AA64ISAR0_EL1_RDM) != 0;
    g_cpu_features.sha3 = (id_aa64isar0 & __ID_AA64ISAR0_EL1_SHA3) != 0;
    g_cpu_features.sm3 = (id_aa64isar0 & __ID_AA64ISAR0_EL1_SM3) != 0;
    g_cpu_features.sm4 = (id_aa64isar0 & __ID_AA64ISAR0_EL1_SM4) != 0;
    g_cpu_features.dot_product = (id_aa64isar0 & __ID_AA64ISAR0_EL1_DP) != 0;
    g_cpu_features.fhm = (id_aa64isar0 & __ID_AA64ISAR0_EL1_FHM) != 0;

    const enum id_aa64isar0_el1_ts_support id_aa64isar0_el1_ts_support =
        (id_aa64isar0 & __ID_AA64ISAR0_EL1_TS) >>
            ID_AA64ISAR0_EL1_TS_SUPPORT_SHIFT;

    switch (id_aa64isar0_el1_ts_support) {
        case ID_AA64ISAR0_EL1_TS_SUPPORT_NONE:
            break;
        case ID_AA64ISAR0_EL1_TS_SUPPORT_FLAG_M:
            g_cpu_features.ts = CPU_FEAT_TS_FLAG_M;
            break;
        case ID_AA64ISAR0_EL1_TS_SUPPORT_FLAG_M2:
            g_cpu_features.ts = CPU_FEAT_TS_FLAG_M2;
            break;
    }

    const enum id_aa64isar0_el1_tlb_support id_aa64isar0_el1_tlb_support =
        (id_aa64isar0 & __ID_AA64ISAR0_EL1_TLB) >>
            ID_AA64ISAR0_EL1_TLB_SUPPORT_SHIFT;

    switch (id_aa64isar0_el1_tlb_support) {
        case ID_AA64ISAR0_EL1_TLB_SUPPORT_NONE:
            break;
        case ID_AA64ISAR0_EL1_TLB_SUPPORT_TLBIOS:
            g_cpu_features.tlb = CPU_FEAT_TLBIOS;
            break;
        case ID_AA64ISAR0_EL1_TLB_SUPPORT_IRANGE:
            g_cpu_features.tlb = CPU_FEAT_TLB_IRANGE;
            break;
    }

    const enum id_aa64isar1_el1_dpb_support id_aa64isar1_el1_dpb_support =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_DPB) >>
            ID_AA64ISAR1_EL1_DPB_SUPPORT_SHIFT;

    switch (id_aa64isar1_el1_dpb_support) {
        case ID_AA64ISAR1_EL1_DPB_SUPPORT_NONE:
            break;
        case ID_AA64ISAR1_EL1_DPB_SUPPORT_FEAT_DPB:
            g_cpu_features.dpb = CPU_FEAT_DPB;
            break;
        case ID_AA64ISAR1_EL1_DPB_SUPPORT_FEAT_DPB2:
            g_cpu_features.dpb = CPU_FEAT_DPB2;
            break;
    }

    g_cpu_features.rndr = (id_aa64isar0 & __ID_AA64ISAR0_EL1_RNDR) != 0;
    const enum id_aa64isar1_el1_apa_support aa64isar1_el1_apa_support =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_APA) >>
            ID_AA64ISAR1_EL1_APA_SUPPORT_SHIFT;
    const enum id_aa64isar1_el1_api_support aa64isar1_el1_api_support =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_API) >>
            ID_AA64ISAR1_EL1_API_SUPPORT_SHIFT;
    const enum id_aa64isar2_el1_apa3_support aa64isar1_el1_apa3_support =
        (id_aa64isar2 & __ID_AA64ISAR2_EL1_APA3) >>
            ID_AA64ISAR2_EL1_APA3_SUPPORT_SHIFT;

    if (aa64isar1_el1_apa_support ==
            ID_AA64ISAR1_EL1_APA_SUPPORT_FEAT_FPACCOMBINE &&
        aa64isar1_el1_api_support ==
            ID_AA64ISAR1_EL1_API_SUPPORT_FEAT_FPACCOMBINE &&
        aa64isar1_el1_apa3_support ==
            ID_AA64ISAR2_EL1_APA3_SUPPORT_FEAT_FPACCOMBINE)
    {
        g_cpu_features.pauth = CPU_FEAT_PAUTH_FPACCOMBINE;
    } else if (aa64isar1_el1_apa_support ==
            ID_AA64ISAR1_EL1_APA_SUPPORT_FEAT_PAUTH2 &&
        aa64isar1_el1_api_support ==
            ID_AA64ISAR1_EL1_API_SUPPORT_FEAT_PAUTH2 &&
        aa64isar1_el1_apa3_support ==
            ID_AA64ISAR2_EL1_APA3_SUPPORT_FEAT_PAUTH2)
    {
        g_cpu_features.pauth = CPU_FEAT_PAUTH2;
    } else if (aa64isar1_el1_apa_support ==
            ID_AA64ISAR1_EL1_APA_SUPPORT_FEAT_EPAC &&
        aa64isar1_el1_api_support ==
            ID_AA64ISAR1_EL1_API_SUPPORT_FEAT_EPAC &&
        aa64isar1_el1_apa3_support ==
            ID_AA64ISAR2_EL1_APA3_SUPPORT_FEAT_EPAC)
    {
        g_cpu_features.pauth = CPU_FEAT_PAUTH_EPAC;
    } else if (aa64isar1_el1_apa_support ==
            ID_AA64ISAR1_EL1_APA_SUPPORT_FEAT_PAUTH &&
        aa64isar1_el1_api_support ==
            ID_AA64ISAR1_EL1_API_SUPPORT_FEAT_PAUTH &&
        aa64isar1_el1_apa3_support ==
            ID_AA64ISAR2_EL1_APA3_SUPPORT_FEAT_PAUTH)
    {
        g_cpu_features.pauth = CPU_FEAT_PAUTH;
    }

    g_cpu_features.jscvt = (id_aa64isar1 & __ID_AA64ISAR1_EL1_JSCVT) != 0;
    g_cpu_features.fcma = (id_aa64isar1 & __ID_AA64ISAR1_EL1_FCMA) != 0;

    const enum id_aa64isar1_el1_lrcpc_support id_aa64isar1_el1_lrcpc_support =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_LRCPC) >>
            ID_AA64ISAR1_EL1_LRCPC_SUPPORT_SHIFT;

    switch (id_aa64isar1_el1_lrcpc_support) {
        case ID_AA64ISAR1_EL1_LRCPC_SUPPORT_NONE:
            break;
        case ID_AA64ISAR1_EL1_LRCPC_SUPPORT_FEAT_LRCPC:
            g_cpu_features.lrcpc = CPU_FEAT_LRCPC;
            break;
        case ID_AA64ISAR1_EL1_LRCPC_SUPPORT_FEAT_LRCPC2:
            g_cpu_features.lrcpc = CPU_FEAT_LRCPC2;
            break;
        case ID_AA64ISAR1_EL1_LRCPC_SUPPORT_FEAT_LRCPC3:
            g_cpu_features.lrcpc = CPU_FEAT_LRCPC3;
            break;
    }

    g_cpu_features.pacqarma5 =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_GPA) &&
        (aa64isar1_el1_apa_support > ID_AA64ISAR1_EL1_APA_SUPPORT_NONE);
    g_cpu_features.pacimp =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_GPI) &&
        (aa64isar1_el1_apa_support > ID_AA64ISAR1_EL1_APA_SUPPORT_NONE);

    g_cpu_features.frintts = (id_aa64isar1 & __ID_AA64ISAR1_EL1_FRINTTS) != 0;
    g_cpu_features.sb = (id_aa64isar1 & __ID_AA64ISAR1_EL1_SB) != 0;

    const enum id_aa64isar1_el1_specres_support specres_support =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_SPECRES) >>
            ID_AA64ISAR1_EL1_SPECRES_SUPPORT_SHIFT;

    switch (specres_support) {
        case ID_AA64ISAR1_EL1_SPECRES_SUPPORT_NONE:
            break;
        case ID_AA64ISAR1_EL1_SPECRES_SUPPORT_FEAT_SPECRES2:
            g_cpu_features.specres = CPU_FEAT_SPECRES;
            break;
        case ID_AA64ISAR1_EL1_SPECRES_SUPPORT_FEAT_SPECRES:
            g_cpu_features.specres = CPU_FEAT_SPECRES2;
            break;
    }

    const enum id_aa64isar1_el1_bf16_support id_aa64isar1_el1_bf16_support =
        (id_aa64isar1 & __ID_AA64ISAR1_EL1_BF16) >>
            ID_AA64ISAR1_EL1_BF16_SUPPORT_SHIFT;

    switch (id_aa64isar1_el1_bf16_support) {
        case ID_AA64ISAR1_EL1_BF16_SUPPORT_NONE:
            break;
        case ID_AA64ISAR1_EL1_BF16_SUPPORT_FEAT_BF16:
            g_cpu_features.bf16 = CPU_FEAT_BF16;
            break;
        case ID_AA64ISAR1_EL1_BF16_SUPPORT_FEAT_EBF16:
            g_cpu_features.bf16 = CPU_FEAT_BF16;
            break;
    }

    g_cpu_features.dgh = (id_aa64isar1 & __ID_AA64ISAR1_EL1_DGH) != 0;
    g_cpu_features.i8mm = (id_aa64isar1 & __ID_AA64ISAR1_EL1_I8MM) != 0;
    g_cpu_features.xs = (id_aa64isar1 & __ID_AA64ISAR1_EL1_XS) != 0;

    const enum id_aa64isar1_el1_ls64_support id_aa64isar1_el1_ls64_support =
        (id_aa64isar1 & ID_AA64ISAR1_EL1_LS64_SUPPORT_SHIFT) >>
            ID_AA64ISAR1_EL1_LS64_SUPPORT_SHIFT;

    switch (id_aa64isar1_el1_ls64_support) {
        case ID_AA64ISAR1_EL1_LS64_SUPPORT_NONE:
            break;
        case ID_AA64ISAR1_EL1_LS64_SUPPORT_FEAT_LS64:
            g_cpu_features.ls64 = CPU_FEAT_LS64;
            break;
        case ID_AA64ISAR1_EL1_LS64_SUPPORT_FEAT_LS64_V:
            g_cpu_features.ls64 = CPU_FEAT_LS64V;
            break;
        case ID_AA64ISAR1_EL1_LS64_SUPPORT_FEAT_LS64_ACCDATA:
            g_cpu_features.ls64 = CPU_FEAT_LS64_ACCDATA;
            break;
    }

    g_cpu_features.wfxt = (id_aa64isar2 & __ID_AA64ISAR2_EL1_WFxT) != 0;
    g_cpu_features.rpres = (id_aa64isar2 & __ID_AA64ISAR2_EL1_RPRES) != 0;
    g_cpu_features.pacqarma3 =
        (id_aa64isar2 & __ID_AA64ISAR2_EL1_GPA3) != 0 &&
        aa64isar1_el1_apa3_support == ID_AA64ISAR2_EL1_APA3_SUPPORT_NONE;

    g_cpu_features.mops = (id_aa64isar2 & __ID_AA64ISAR2_EL1_MOPS) != 0;
    g_cpu_features.hbc = (id_aa64isar2 & __ID_AA64ISAR2_EL1_BC) != 0;
    g_cpu_features.clrbhb = (id_aa64isar2 & __ID_AA64ISAR2_EL1_CLRBHB) != 0;
    g_cpu_features.sysreg128 =
        (id_aa64isar2 & __ID_AA64ISAR2_EL1_SYSREG_128) != 0;
    g_cpu_features.sysinstr128 =
        (id_aa64isar2 & __ID_AA64ISAR2_EL1_SYSINSTR_128) != 0;
    g_cpu_features.prfmslc = (id_aa64isar2 & __ID_AA64ISAR2_EL1_PRFMSLC) != 0;

    const enum id_aa64pfr0_el1_fp_support id_aa64pfr0_el1_fp_support =
        (id_aa64pfr0 & __ID_AA64PFR0_EL1_FP) >>
            ID_AA64PFR0_EL1_FP_SUPPORT_SHIFT;

    switch (id_aa64pfr0_el1_fp_support) {
        case ID_AA64PFR0_EL1_FP_SUPPORT_PARTIAL:
            g_cpu_features.fp = CPU_FEAT_FP_PARTIAL;
            g_cpu_features.fp16 = true;

            break;
        case ID_AA64PFR0_EL1_FP_SUPPORT_FULL:
            g_cpu_features.fp = CPU_FEAT_FP_FULL;
            g_cpu_features.fp16 = true;

            break;
        case ID_AA64PFR0_EL1_FP_SUPPORT_NONE:
            break;
    }

    const enum id_aa64pfr0_el1_adv_simd_support advsimd_support =
        (id_aa64pfr0 & __ID_AA64PFR0_EL1_ADV_SIMD) >>
            ID_AA64PFR0_EL1_ADV_SIMD_SUPPORT_SHIFT;

    switch (advsimd_support) {
        case ID_AA64PFR0_EL1_ADV_SIMD_SUPPORT_PARTIAL:
            g_cpu_features.adv_simd = CPU_FEAT_ADV_SIMD_PARTIAL;
            break;
        case ID_AA64PFR0_EL1_ADV_SIMD_SUPPORT_FULL:
            g_cpu_features.adv_simd = CPU_FEAT_ADV_SIMD_FULL;
            g_cpu_features.fp16 = true;

            break;
        case ID_AA64PFR0_EL1_ADV_SIMD_SUPPORT_NONE:
            break;
    }

    const enum id_aa64pfr0_el1_gic_support id_aa64pfr0_el1_gic_support =
        (id_aa64pfr0 & ID_AA64PFR0_EL1_GIC_SUPPORT_SHIFT) >>
            ID_AA64PFR0_EL1_GIC_SUPPORT_SHIFT;

    switch (id_aa64pfr0_el1_gic_support) {
        case ID_AA64PFR0_EL1_GIC_SUPPORT_NONE:
            break;
        case ID_AA64PFR0_EL1_GIC_SUPPORT_V4:
            g_cpu_features.gic = CPU_FEAT_GIC_V4;
            break;
        case ID_AA64PFR0_EL1_GIC_SUPPORT_V4p1:
            g_cpu_features.gic = CPU_FEAT_GIC_V4p1;
            break;
    }

    const enum id_aa64pfr0_el1_ras_support id_aa64pfr0_el1_ras_support =
        (id_aa64pfr0 & ID_AA64PFR0_EL1_RAS_SUPPORT_SHIFT) >>
            ID_AA64PFR0_EL1_RAS_SUPPORT_SHIFT;

    switch (id_aa64pfr0_el1_ras_support) {
        case ID_AA64PFR0_EL1_RAS_SUPPORT_NONE:
        case ID_AA64PFR0_EL1_RAS_SUPPORT_FEAT_RAS:
            g_cpu_features.ras = CPU_FEAT_RAS;
            break;
        case ID_AA64PFR0_EL1_RAS_SUPPORT_FEAT_RASv1p1:
            g_cpu_features.ras = CPU_FEAT_RASv1p1;
            break;
        case ID_AA64PFR0_EL1_RAS_SUPPORT_FEAT_RASv2:
            g_cpu_features.ras = CPU_FEAT_RASv2;
            break;
    }

    #if 0
    const enum id_aa64zfr0_el1_sve_support id_aa64zfr0_el1_sve_support =
        (id_aa64zfr0 & __ID_AA64ZFR0_EL1_SVEVER) >>
            ID_AA64ZFR0_EL1_SVE_SUPPORT_SHIFT;

    switch (id_aa64zfr0_el1_sve_support) {
        case ID_AA64ZFR0_EL1_SVE_SUPPORT_SVE:
            g_cpu_features.sve = CPU_FEAT_SVE;
            break;
        case ID_AA64ZFR0_EL1_SVE_SUPPORT_SVE2:
            g_cpu_features.sve = CPU_FEAT_SVE2;
            break;
        case ID_AA64ZFR0_EL1_SVE_SUPPORT_SVE2p1:
            g_cpu_features.sve = CPU_FEAT_SVE2p1;
            break;
    }

    const enum id_aa64zfr0_el1_sve_aes_support id_aa64zfr0_el1_sve_aes_support =
        (id_aa64zfr0 & __ID_AA64ZFR0_EL1_SVE_AES) >>
            ID_AA64ZFR0_EL1_SVE_AES_SUPPORT_SHIFT;

    switch (id_aa64zfr0_el1_sve_aes_support) {
        case ID_AA64ZFR0_EL1_SVE_AVS_SUPPORT_NONE:
            break;
        case ID_AA64ZFR0_EL1_SVE_AVS_SUPPORT_FEAT_AES:
            g_cpu_features.sve_aes = CPU_FEAT_SVE_AES;
            break;
        case ID_AA64ZFR0_EL1_SVE_AES_SUPPORT_FEAT_SVE_PMULL128:
            g_cpu_features.sve_aes = CPU_FEAT_SVE_AES_PMULL128;
            break;
    }

    const enum id_aa64zfr0_el1_sve_bf16_support sve_bf16_support =
        (id_aa64zfr0 & __ID_AA64ZFR0_EL1_BF16) >>
            ID_AA64ZFR0_EL1_SVE_BF16_SUPPORT_SHIFT;

    switch (sve_bf16_support) {
        case ID_AA64ZFR0_EL1_SVE_BF16_SUPPORT_NONE:
        case ID_AA64ZFR0_EL1_SVE_BF16_SUPPORT_FEAT_BF16:
            g_cpu_features.sve_bf16 = CPU_FEAT_SVE_BF16;
            break;
        case ID_AA64ZFR0_EL1_SVE_BF16_SUPPORT_FEAT_EBF16:
            g_cpu_features.sve_bf16 = CPU_FEAT_SVE_EBF16;
            break;
    }
    #endif

    g_cpu_features.sel2 = (id_aa64pfr0 & __ID_AA64PFR0_EL1_SEL2) != 0;
    const enum id_aa64pfr0_el1_amu_support id_aa64pfr0_el1_amu_support =
        (id_aa64pfr0 & __ID_AA64ZFR0_EL1_BF16) >>
            ID_AA64PFR0_EL1_AMU_SUPPORT_SHIFT;

    switch (id_aa64pfr0_el1_amu_support) {
        case ID_AA64PFR0_EL1_AMU_SUPPORT_NONE:
            break;
        case ID_AA64PFR0_EL1_AMU_SUPPORT_FEAT_AMUv1:
            g_cpu_features.amu = CPU_FEAT_AMU_AMUv1;
            break;
        case ID_AA64PFR0_EL1_AMU_SUPPORT_FEAT_AMUv1p1:
            g_cpu_features.amu = CPU_FEAT_AMU_AMUv1p1;
            break;
    }

    g_cpu_features.dit = (id_aa64pfr0 & __ID_AA64PFR0_EL1_DIT) != 0;
    g_cpu_features.rme = (id_aa64pfr0 & __ID_AA64PFR0_EL1_RME) != 0;

    const enum id_aa64pfr0_el1_csv2_support id_aa64pfr0_el1_csv2_support =
        (id_aa64pfr0 & __ID_AA64PFR0_EL1_CSV2) >>
            ID_AA64PFR0_EL1_CSV2_SUPPORT_SHIFT;

    switch (id_aa64pfr0_el1_csv2_support) {
        case ID_AA64PFR0_EL1_CSV2_SUPPORT_NONE:
            break;
        case ID_AA64PFR0_EL1_CSV2_SUPPORT_FEAT_CSV2:
            g_cpu_features.csv2 = CPU_FEAT_CSV2;
            break;
        case ID_AA64PFR0_EL1_CSV2_SUPPORT_FEAT_CSV2_2:
            g_cpu_features.csv2 = CPU_FEAT_CSV2_2;
            break;
        case ID_AA64PFR0_EL1_CSV2_SUPPORT_FEAT_CSV2_3:
            g_cpu_features.csv2 = CPU_FEAT_CSV2_3;
            break;
    }

    g_cpu_features.csv3 = (id_aa64pfr0 & __ID_AA64PFR0_EL1_CSV3) != 0;
    g_cpu_features.bti = (id_aa64pfr0 & __ID_AA64PFR1_EL1_BT) != 0;

    const enum id_aa64pfr1_el1_ssbs_support id_aa64pfr1_el1_ssbs_support =
        (id_aa64pfr1 & __ID_AA64PFR1_EL1_SSBS) >>
            ID_AA64PFR1_EL1_SSBS_SUPPORT_SHIFT;

    switch (id_aa64pfr1_el1_ssbs_support) {
        case ID_AA64PFR1_EL1_SSBS_SUPPORT_NONE:
            break;
        case ID_AA64PFR1_EL1_SSBS_SUPPORT_FEAT_SSBS:
            g_cpu_features.ssbs = CPU_FEAT_SSBS;
            break;
        case ID_AA64PFR1_EL1_SSBS_SUPPORT_FEAT_SSBS2:
            g_cpu_features.ssbs = CPU_FEAT_SSBS2;
            break;
    }

    const enum id_aa64pfr1_el1_mte_support id_aa64pfr1_el1_mte_support =
        (id_aa64pfr1 & __ID_AA64PFR1_EL1_MTE) >>
            ID_AA64PFR1_EL1_MTE_SUPPORT_SHIFT;

    switch (id_aa64pfr1_el1_mte_support) {
        case ID_AA64PFR1_EL1_MTE_SUPPORT_NONE:
            break;
        case ID_AA64PFR1_EL1_MTE_SUPPORT_FEAT_MTE:
            g_cpu_features.mte = CPU_FEAT_MTE;
            break;
        case ID_AA64PFR1_EL1_MTE_SUPPORT_FEAT_MTE2:
            g_cpu_features.mte = CPU_FEAT_MTE2;
            break;
        case ID_AA64PFR1_EL1_MTE_SUPPORT_FEAT_MTE3:
            g_cpu_features.mte = CPU_FEAT_MTE3;
            break;
        default:
            if (id_aa64pfr1_el1_mte_support >
                    ID_AA64PFR1_EL1_MTE_SUPPORT_FEAT_MTE3)
            {
                if (id_aa64pfr1 & __ID_AA64PFR1_EL1_MTEX) {
                    g_cpu_features.mte = CPU_FEAT_MTEX;
                }
            }

            break;
    }

    const enum id_aa64mmfr0_sme_version id_aa64mmfr0_sme_version =
        (id_aa64mmfr0 & __ID_AA64SMFR0_SMEVER) >>
            ID_AA64MMFR0_SME_VERSION_SHIFT;

    switch (id_aa64mmfr0_sme_version) {
        case ID_AA64MMFR0_SME_VERSION_FEAT_SME:
            g_cpu_features.sme = CPU_FEAT_SME;
            break;
        case ID_AA64MMFR0_SME_VERSION_FEAT_SME2:
            g_cpu_features.sme = CPU_FEAT_SME2;
            break;
        case ID_AA64MMFR0_SME_VERSION_FEAT_SME2p1:
            g_cpu_features.sme = CPU_FEAT_SME2p1;
            break;
    }

    g_cpu_features.rndr_trap = (id_aa64pfr1 & __ID_AA64PFR1_EL1_RNDR_TRAP) != 0;
    g_cpu_features.nmi = (id_aa64pfr1 & __ID_AA64PFR1_EL1_NMI) != 0;
    g_cpu_features.gcs = (id_aa64pfr1 & __ID_AA64PFR1_EL1_GCS) != 0;
    g_cpu_features.the = (id_aa64pfr1 & __ID_AA64PFR1_EL1_THE) != 0;
    g_cpu_features.pfar = (id_aa64pfr1 & __ID_AA64PFR1_EL1_PFAR) != 0;
    g_cpu_features.pa_range =
        (id_aa64mmfr0 & __ID_AA64MMFR0_PARANGE) >> ID_AA64MMFR0_PA_RANGE_SHIFT;

    if (g_cpu_features.pa_range == ID_AA64MMFR0_PA_RANGE_52B_4PIB) {
        if (tcr_el1 & __TCR_DS) {
            g_cpu_features.lpa2 = true;
        } else {
            g_cpu_features.lpa = true;
        }
    }

    g_cpu_features.exs = (id_aa64mmfr0 & __ID_AA64MMFR0_EXS) != 0;
    const enum id_aa64mmfr0_fgt_support id_aa64mmfr0_fgt_support =
        (id_aa64mmfr0 & __ID_AA64MMFR0_FGT) >> ID_AA64MMFR0_FGT_SUPPORT_SHIFT;

    switch (id_aa64mmfr0_fgt_support) {
        case ID_AA64MMFR0_FGT_SUPPORT_NONE:
            break;
        case ID_AA64MMFR0_FGT_SUPPORT_FEAT_FGT:
            g_cpu_features.fgt = CPU_FEAT_FGT;
            break;
        case ID_AA64MMFR0_FGT_SUPPORT_FEAT_FGT2:
            g_cpu_features.fgt = CPU_FEAT_FGT2;
            break;
    }

    if (id_aa64mmfr0 & __ID_AA64MMFR0_ECV) {
        const enum id_aa64mmfr0_ecv_support id_aa64mmfr0_ecv_support =
            (id_aa64mmfr0 & __ID_AA64MMFR0_ECV) >>
                ID_AA64MMFR0_ECV_SUPPORT_SHIFT;

        switch (id_aa64mmfr0_ecv_support) {
            case ID_AA64MMFR0_ECV_SUPPORT_NONE:
                break;
            case ID_AA64MMFR0_ECV_SUPPORT_FEAT_ECV_PARTIAL:
                g_cpu_features.ecv = CPU_FEAT_ECV_PARTIAL;
                break;
            case ID_AA64MMFR0_ECV_SUPPORT_FEAT_ECV_FULL:
                g_cpu_features.ecv = CPU_FEAT_ECV_FULL;
                break;
        }
    }

    const enum id_aa64mmfr1_hafdbs_support id_aa64mmfr1_hafdbs_support =
        (id_aa64mmfr1 & __ID_AA64MMFR1_HAFDBS) >>
            ID_AA64MMFR1_HAFDBS_SUPPORT_SHIFT;

    switch (id_aa64mmfr1_hafdbs_support) {
        case ID_AA64MMFR1_HAFDBS_SUPPORT_NONE:
            break;
        case ID_AA64MMFR1_HAFDBS_SUPPORT_FEAT_HAFDBS_ACCESS_BIT:
            g_cpu_features.hafdbs = CPU_FEAT_HAFDBS_ONLY_ACCESS_BIT;
            break;
        case ID_AA64MMFR1_HAFDBS_SUPPORT_FEAT_HAFDBS_ACCESS_AND_DIRTY:
            g_cpu_features.hafdbs = CPU_FEAT_HAFDBS_ACCESS_AND_DIRTY_BIT;
            break;
        case ID_AA64MMFR1_HAFDBS_SUPPORT_FEAT_HAFT:
            g_cpu_features.hafdbs = CPU_FEAT_HAFDBS_FULL;
            break;
    }

    g_cpu_features.vmid16 = (id_aa64mmfr1 & __ID_AA64MMFR1_VMIDBITS) != 0;
    g_cpu_features.haft = (id_aa64mmfr1 & __ID_AA64MMFR1_VH) != 0;

    const enum id_aa64mmfr1_hpds_support id_aa64mmfr1_hpds_support =
        (id_aa64mmfr1 & __ID_AA64MMFR1_HPDS) >> ID_AA64MMFR1_HPDS_SUPPORT_SHIFT;

    switch (id_aa64mmfr1_hpds_support) {
        case ID_AA64MMFR1_HPDS_SUPPORT_NONE:
            break;
        case ID_AA64MMFR1_HPDS_SUPPORT_FEAT_HPDS:
            g_cpu_features.hpds = CPU_FEAT_HPDS;
            break;
        case ID_AA64MMFR1_HPDS_SUPPORT_FEAT_HPDS2:
            g_cpu_features.hpds = CPU_FEAT_HPDS2;
            break;
    }

    g_cpu_features.lor = (id_aa64mmfr1 & __ID_AA64MMFR1_LO) != 0;

    const enum id_aa64mmfr1_pan_support id_aa64mmfr1_pan_support =
        (id_aa64mmfr1 & __ID_AA64MMFR1_PAN) >> ID_AA64MMFR1_PAN_SUPPORT_SHIFT;

    switch (id_aa64mmfr1_pan_support) {
        case ID_AA64MMFR1_PAN_SUPPORT_NONE:
            break;
        case ID_AA64MMFR1_PAN_SUPPORT_FEAT_PAN:
            g_cpu_features.pan = CPU_FEAT_PAN;
            break;
        case ID_AA64MMFR1_PAN_SUPPORT_FEAT_PAN2:
            g_cpu_features.pan = CPU_FEAT_PAN2;
            break;
        case ID_AA64MMFR1_PAN_SUPPORT_FEAT_PAN3:
            g_cpu_features.pan = CPU_FEAT_PAN3;
            break;
    }

    g_cpu_features.xnx = (id_aa64mmfr1 & __ID_AA64MMFR1_XNX) != 0;
    g_cpu_features.twed = (id_aa64mmfr1 & __ID_AA64MMFR1_TWED) != 0;
    g_cpu_features.ets = (id_aa64mmfr1 & __ID_AA64MMFR1_ETS) != 0;
    g_cpu_features.hcx = (id_aa64mmfr1 & __ID_AA64MMFR1_HCX) != 0;
    g_cpu_features.afp = (id_aa64mmfr1 & __ID_AA64MMFR1_AFP) != 0;
    g_cpu_features.ntlbpa = (id_aa64mmfr2 & __ID_AA64MMFR1_NTLBPA) != 0;
    g_cpu_features.tidcp1 = (id_aa64mmfr2 & __ID_AA64MMFR1_TIDCP1) != 0;
    g_cpu_features.cmow = (id_aa64mmfr2 & __ID_AA64MMFR1_CMOW) != 0;
    g_cpu_features.ecbhb = (id_aa64mmfr2 & __ID_AA64MMFR1_ECBHB) != 0;
    g_cpu_features.ttcnp = (id_aa64mmfr2 & __ID_AA64MMFR2_CNP) != 0;
    g_cpu_features.uao = (id_aa64mmfr2 & __ID_AA64MMFR2_UAO) != 0;
    g_cpu_features.lsmaoc = (id_aa64mmfr2 & __ID_AA64MMFR2_LSM) != 0;
    g_cpu_features.iesb = (id_aa64mmfr2 & __ID_AA64MMFR2_IESB) != 0;
    g_cpu_features.ccidx = (id_aa64mmfr2 & __ID_AA64MMFR2_CCIDX) != 0;
    g_cpu_features.lva =
        (id_aa64mmfr2 & __ID_AA64MMFR2_VARANGE) != 0 ||
        (tcr_el1 & __TCR_DS) != 0;

    const enum id_aa64mmfr2_nv_support id_aa64mmfr2_nv_support =
        (id_aa64mmfr2 & __ID_AA64MMFR2_NV) >> ID_AA64MMFR2_NV_SUPPORT_SHIFT;

    switch (id_aa64mmfr2_nv_support) {
        case ID_AA64MMFR2_NV_SUPPORT_NONE:
            break;
        case ID_AA64MMFR2_NV_SUPPORT_FEAT_NV:
            g_cpu_features.nv = CPU_FEAT_NV;
            break;
        case ID_AA64MMFR2_NV_SUPPORT_FEAT_NV2:
            g_cpu_features.nv = CPU_FEAT_NV2;
            break;
    }

    g_cpu_features.ttst = (id_aa64mmfr2 & __ID_AA64MMFR2_ST) != 0;
    g_cpu_features.lse2 = (id_aa64mmfr2 & __ID_AA64MMFR2_AT) != 0;
    g_cpu_features.idst = (id_aa64mmfr2 & __ID_AA64MMFR2_IDS) != 0;
    g_cpu_features.s2fwb = (id_aa64mmfr2 & __ID_AA64MMFR2_FWB) != 0;
    g_cpu_features.ttl = (id_aa64mmfr2 & __ID_AA64MMFR2_TTL) != 0;

    const enum id_aa64mmfr2_bbm_support id_aa64mmfr2_bbm_support =
        (id_aa64mmfr2 & __ID_AA64MMFR2_BBM) >> ID_AA64MMFR2_BBM_SUPPORT_SHIFT;

    switch (id_aa64mmfr2_bbm_support) {
        case ID_AA64MMFR2_BBM_SUPPORT_LEVEL0:
            g_cpu_features.bbm = CPU_FEAT_BBM_LVL0;
            break;
        case ID_AA64MMFR2_BBM_SUPPORT_LEVEL1:
            g_cpu_features.bbm = CPU_FEAT_BBM_LVL1;
            break;
        case ID_AA64MMFR2_BBM_SUPPORT_LEVEL2:
            g_cpu_features.bbm = CPU_FEAT_BBM_LVL2;
            break;
    }

    g_cpu_features.evt = (id_aa64mmfr2 & __ID_AA64MMFR2_EVT) != 0;
    g_cpu_features.e0pd = (id_aa64mmfr2 & __ID_AA64MMFR2_E0PD) != 0;

    if (g_cpu_features.e0pd) {
        g_cpu_features.csv3 = true;
    }

    // FIXME: gcc doesn't register the id_aa64smfr0 register
#if 0
    const enum id_aa64mmfr3_anerr_support id_aa64mmfr3_anerr_support =
        (id_aa64mmfr3 & __ID_AA64MMFR3_ANERR) >>
            ID_AA64MMFR3_ANERR_SUPPORT_SHIFT;
    const enum id_aa64mmfr3_snerr_support id_aa64mmfr3_snerr_support =
        (id_aa64mmfr3 & __ID_AA64MMFR3_SNERR) >>
            ID_AA64MMFR3_SNERR_SUPPORT_SHIFT;

    if (id_aa64mmfr3_anerr_support == ID_AA64MMFR3_ANERR_SUPPORT_FEAT_ANERR &&
        id_aa64mmfr3_snerr_support == ID_AA64MMFR3_SNERR_SUPPORT_FEAT_ANERR)
    {
        g_cpu_features.anerr = true;
    }

    const enum id_aa64mmfr3_aderr_support id_aa64mmfr3_aderr_support =
        (id_aa64mmfr3 & __ID_AA64MMFR3_ADERR) >>
            ID_AA64MMFR3_ADERR_SUPPORT_SHIFT;
    const enum id_aa64mmfr3_sderr_support id_aa64mmfr3_sderr_support =
        (id_aa64mmfr3 & __ID_AA64MMFR3_SDERR) >>
            ID_AA64MMFR3_SDERR_SUPPORT_SHIFT;

    if (id_aa64mmfr3_aderr_support == ID_AA64MMFR3_ADERR_SUPPORT_FEAT_ADERR &&
        id_aa64mmfr3_sderr_support == ID_AA64MMFR3_SDERR_SUPPORT_FEAT_ADERR)
    {
        g_cpu_features.aderr = true;
    }

    g_cpu_features.aie = (id_aa64mmfr3 & __ID_AA64MMFR3_AIE) != 0;
    g_cpu_features.mec = (id_aa64mmfr3 & __ID_AA64MMFR3_MEC) != 0;
    g_cpu_features.s1pie = (id_aa64mmfr3 & __ID_AA64MMFR3_S1PIE) != 0;
    g_cpu_features.s2pie = (id_aa64mmfr3 & __ID_AA64MMFR3_S2PIE) != 0;
    g_cpu_features.s1poe = (id_aa64mmfr3 & __ID_AA64MMFR3_S1POE) != 0;
    g_cpu_features.s2poe = (id_aa64mmfr3 & __ID_AA64MMFR3_S2POE) != 0;
    g_cpu_features.sve_bitperm = (id_aa64zfr0 & __ID_AA64ZFR0_EL1_BITPERM) != 0;

    g_cpu_features.sme_f16f16 = (id_aa64smfr0 & __ID_AA64SMFR0_F16F16) != 0;
    g_cpu_features.sme_f64f64 = (id_aa64smfr0 & __ID_AA64SMFR0_F64F64) != 0;
    g_cpu_features.sme_i16i64 = (id_aa64smfr0 & __ID_AA64SMFR0_I16I64) != 0;

    if (g_cpu_features.sme != CPU_FEAT_NONE) {
        g_cpu_features.sme_fa64 = (id_aa64smfr0 & __ID_AA64SMFR0_FA64) != 0;
    }

    g_cpu_features.b16b16 =
        (id_aa64zfr0 & __ID_AA64ZFR0_EL1_B16B16) != 0 &&
        (id_aa64smfr0 & __ID_AA64SMFR0_B16B16) != 0;
#endif

    //g_cpu_features.sve_sha3 = (id_aa64zfr0 & __ID_AA64ZFR0_EL1_SHA3) != 0;
    //g_cpu_features.sve_sm4 = (id_aa64zfr0 & __ID_AA64ZFR0_EL1_SM4) != 0;
    g_cpu_features.i8mm = (id_aa64isar1 & __ID_AA64ISAR1_EL1_I8MM) != 0;
    g_cpu_features.xs = (id_aa64isar1 & __ID_AA64ISAR1_EL1_XS) != 0;
    g_cpu_features.ls64 = (id_aa64isar1 & __ID_AA64ISAR1_EL1_LS64) != 0;

    const enum id_aa64dfr0_el1_debug_version_support debug_version_support =
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_DEBUG_VERSION) >>
            ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_SHIFT;

    switch (debug_version_support) {
        case ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_DEFAULT:
            g_cpu_features.debug = CPU_FEAT_DEBUG_DEFAULT;
            break;
        case ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_FEAT_VHE:
            g_cpu_features.debug = CPU_FEAT_DEBUG_VHE;
            break;
        case ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_FEAT_Debugv8p2:
            g_cpu_features.debug = CPU_FEAT_DEBUG_v8p2;
            break;
        case ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_FEAT_Debugv8p4:
            g_cpu_features.debug = CPU_FEAT_DEBUG_v8p4;
            break;
        case ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_FEAT_Debugv8p8:
            g_cpu_features.debug = CPU_FEAT_DEBUG_v8p8;
            break;
        case ID_AA64DFR0_EL1_DEBUG_VERSION_SUPPORT_FEAT_Debugv8p9:
            g_cpu_features.debug = CPU_FEAT_DEBUG_v8p9;
            break;
    }

    const enum id_aa64dfr0_el1_pmu_support id_aa64dfr0_el1_pmu_support =
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_PMUVER) >>
            IO_AA64DFR0_EL1_PMUVER_SHIFT;

    switch (id_aa64dfr0_el1_pmu_support) {
        case ID_AA64DFR0_EL1_PMU_SUPPORT_NONE:
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3;
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3p1:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3p1;
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3p4:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3p4;
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3p5:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3p5;
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3p7:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3p7;
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3p8:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3p8;
            break;
        case ID_AA64DFR0_EL1_PMU_SUPPORT_FEAT_PMUv3p9:
            g_cpu_features.pmu = CPU_FEAT_PMU_FEAT_PMUv3p9;
            break;
    }

    g_cpu_features.pmuv3_ss = (id_aa64dfr0 & __ID_AA64DFR0_EL1_PMSS) != 0;
    g_cpu_features.sebep = (id_aa64dfr0 & __ID_AA64DFR0_EL1_SEBEP) != 0;
    g_cpu_features.double_lock =
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_DOUBLELOCK) != 0;

    g_cpu_features.trf = (id_aa64dfr0 & __ID_AA64DFR0_EL1_TRACEFILT) != 0;
    g_cpu_features.trbe = (id_aa64dfr0 & __ID_AA64DFR0_EL1_TRACEBUFFER) != 0;

    const enum id_aa64dfr0_el1_mtpmu_support id_aa64dfr0_el1_mtpmu_support =
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_MTPMU) >> IO_AA64DFR0_EL1_MTPMU_SHIFT;

    switch (id_aa64dfr0_el1_mtpmu_support) {
        case ID_AA64DFR0_EL1_MTPMU_SUPPORT_NONE:
            break;
        case ID_AA64DFR0_EL1_MTPMU_SUPPORT_FEAT_MTPMU:
            g_cpu_features.mtpmu = CPU_FEAT_MTPMU;
            break;
        case ID_AA64DFR0_EL1_MTPMU_SUPPORT_FEAT_MTPMU_3_MAYBE:
            g_cpu_features.mtpmu = CPU_FEAT_MTPMU_3_MAYBE;
            break;
    }

    const enum id_aa64dfr0_el1_brbe_support id_aa64dfr0_el1_brbe_support =
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_BRBE) >> IO_AA64DFR0_EL1_MTPMU_SHIFT;

    switch (id_aa64dfr0_el1_brbe_support) {
        case ID_AA64DFR0_EL1_BRBE_SUPPORT_NONE:
            break;
        case ID_AA64DFR0_EL1_BRBE_SUPPORT_FEAT_BRBE:
            g_cpu_features.brbe = CPU_FEAT_BRBE;
            break;
        case ID_AA64DFR0_EL1_BRBE_SUPPORT_FEAT_BRBEv1p1:
            g_cpu_features.brbe = CPU_FEAT_BRBEv1p1;
            break;
    }

    g_cpu_features.trbe_ext =
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_EXTTRCBUFF) != 0 &&
        (id_aa64dfr0 & __ID_AA64DFR0_EL1_TRACEBUFFER) != 0;

    g_cpu_features.hpmn0 = (id_aa64dfr0 & __ID_AA64DFR0_EL1_HPMN0) != 0;
    g_cpu_features.able = (id_aa64dfr1 & __ID_AA64DFR1_EL1_ABLE) != 0;
    g_cpu_features.ebep = (id_aa64dfr1 & __ID_AA64DFR1_EL1_EBEP) != 0;
    g_cpu_features.ite = (id_aa64dfr1 & __ID_AA64DFR1_EL1_ITE) != 0;
    g_cpu_features.pmuv3_icntr = (id_aa64dfr1 & __ID_AA64DFR1_EL1_PMICNTR) != 0;
    g_cpu_features.spmu = (id_aa64dfr1 & __ID_AA64DFR1_EL1_SPMU) != 0;

    if (!g_cpu_features.b16b16) {
        g_cpu_features.sve = CPU_FEAT_SVE;
    }

    if (!g_cpu_features.sme_f16f16) {
        if (g_cpu_features.sme > CPU_FEAT_SME2) {
            g_cpu_features.sme = CPU_FEAT_SME2;
        }
    }
}

void print_cpu_features() {
    const char *pauth_string = NULL;
    switch (g_cpu_features.pauth) {
        case CPU_FEAT_PAUTH_NONE:
            pauth_string = "none";
            break;
        case CPU_FEAT_PAUTH:
            pauth_string = "pauth";
            break;
        case CPU_FEAT_PAUTH_EPAC:
            pauth_string = "pauth-epac";
            break;
        case CPU_FEAT_PAUTH2:
            pauth_string = "pauth2";
            break;
        case CPU_FEAT_PAUTH_FPACCOMBINE:
            pauth_string = "pauth-fpaccombine";
            break;
    }

    const char *lrcpc_string = NULL;
    switch (g_cpu_features.lrcpc) {
        case CPU_FEAT_LRCPC_NONE:
            lrcpc_string = "none";
            break;
        case CPU_FEAT_LRCPC:
            lrcpc_string = "lrcpc";
            break;
        case CPU_FEAT_LRCPC2:
            lrcpc_string = "lrcpc2";
            break;
        case CPU_FEAT_LRCPC3:
            lrcpc_string = "lrcpc3";
            break;
    }

    const char *ls64_string = NULL;
    switch (g_cpu_features.ls64) {
        case CPU_FEAT_LS64_NONE:
            ls64_string = "none";
            break;
        case CPU_FEAT_LS64:
            ls64_string = "ls64";
            break;
        case CPU_FEAT_LS64V:
            ls64_string = "ls64-v";
            break;
        case CPU_FEAT_LS64_ACCDATA:
            ls64_string = "ls64-accdata";
            break;
    }

    const char *ras_string = NULL;
    switch (g_cpu_features.ras) {
        case CPU_FEAT_RAS_NONE:
            ras_string = "none";
            break;
        case CPU_FEAT_RAS:
            ras_string = "ras";
            break;
        case CPU_FEAT_RASv1p1:
            ras_string = "ras-v1.1";
            break;
        case CPU_FEAT_RASv2:
            ras_string = "ras-v2";
            break;
    }

    const char *csv2_string = NULL;
    switch (g_cpu_features.csv2) {
        case CPU_FEAT_CSV2_NONE:
            csv2_string = "none";
            break;
        case CPU_FEAT_CSV2:
            csv2_string = "csv2";
            break;
        case CPU_FEAT_CSV2_2:
            csv2_string = "csv2-v2";
            break;
        case CPU_FEAT_CSV2_3:
            csv2_string = "csv2-v3";
            break;
    }

    const char *mte_string = NULL;
    switch (g_cpu_features.mte) {
        case CPU_FEAT_MTE_NONE:
            mte_string = "none";
            break;
        case CPU_FEAT_MTE:
            mte_string = "mte";
            break;
        case CPU_FEAT_MTE2:
            mte_string = "mte2";
            break;
        case CPU_FEAT_MTE3:
            mte_string = "mte3";
            break;
        case CPU_FEAT_MTEX:
            mte_string = "mtex";
            break;
    }

    const char *sme_string = NULL;
    switch (g_cpu_features.sme) {
        case CPU_FEAT_SME_NONE:
            sme_string = "none";
            break;
        case CPU_FEAT_SME:
            sme_string = "sme";
            break;
        case CPU_FEAT_SME2:
            sme_string = "sme2";
            break;
        case CPU_FEAT_SME2p1:
            sme_string = "sme2.1";
            break;
    }

    const char *parange_string = NULL;
    switch (g_cpu_features.pa_range) {
        case ID_AA64MMFR0_PA_RANGE_32B_4GIB:
            parange_string = "32-bit, 4gib";
            break;
        case ID_AA64MMFR0_PA_RANGE_36B_64GIB:
            parange_string = "36-bit, 64gib";
            break;
        case ID_AA64MMFR0_PA_RANGE_40B_1TIB:
            parange_string = "40-bit, 1tib";
            break;
        case ID_AA64MMFR0_PA_RANGE_42B_4TIB:
            parange_string = "42-bit, 4tib";
            break;
        case ID_AA64MMFR0_PA_RANGE_44B_16TIB:
            parange_string = "44-bit, 16tib";
            break;
        case ID_AA64MMFR0_PA_RANGE_48B_256TIB:
            parange_string = "48-bit, 256tib";
            break;
        case ID_AA64MMFR0_PA_RANGE_52B_4PIB:
            parange_string = "52-bit, 4pib";
            break;
        case ID_AA64MMFR0_PA_RANGE_56B_64PIB:
            parange_string = "56-bit, 64pib";
            break;
    }

    const char *hafdbs_string = NULL;
    switch (g_cpu_features.hafdbs) {
        case CPU_FEAT_HAFDBS_NONE:
            hafdbs_string = "none";
            break;
        case CPU_FEAT_HAFDBS_ONLY_ACCESS_BIT:
            hafdbs_string = "only-access-bit";
            break;
        case CPU_FEAT_HAFDBS_ACCESS_AND_DIRTY_BIT:
            hafdbs_string = "access-and-dirty";
            break;
        case CPU_FEAT_HAFDBS_FULL:
            hafdbs_string = "full";
            break;
    }

    const char *pan_string = NULL;
    switch (g_cpu_features.pan) {
        case CPU_FEAT_PAN_NONE:
            pan_string = "none";
            break;
        case CPU_FEAT_PAN:
            pan_string = "pan";
            break;
        case CPU_FEAT_PAN2:
            pan_string = "pan2";
            break;
        case CPU_FEAT_PAN3:
            pan_string = "pan3";
            break;
    }

    const char *debug_string = NULL;
    switch (g_cpu_features.debug) {
        case CPU_FEAT_DEBUG_DEFAULT:
            debug_string = "default";
            break;
        case CPU_FEAT_DEBUG_VHE:
            debug_string = "vhe";
            break;
        case CPU_FEAT_DEBUG_v8p2:
            debug_string = "v8.2";
            break;
        case CPU_FEAT_DEBUG_v8p4:
            debug_string = "v8.4";
            break;
        case CPU_FEAT_DEBUG_v8p8:
            debug_string = "v8.8";
            break;
        case CPU_FEAT_DEBUG_v8p9:
            debug_string = "v8.9";
            break;
    }

    const char *pmu_string = NULL;
    switch (g_cpu_features.pmu) {
        case CPU_FEAT_PMU_FEAT_PMUv3:
            pmu_string = "v3";
            break;
        case CPU_FEAT_PMU_FEAT_PMUv3p1:
            pmu_string = "v3.1";
            break;
        case CPU_FEAT_PMU_FEAT_PMUv3p4:
            pmu_string = "v3.4";
            break;
        case CPU_FEAT_PMU_FEAT_PMUv3p5:
            pmu_string = "v3.5";
            break;
        case CPU_FEAT_PMU_FEAT_PMUv3p7:
            pmu_string = "v3.7";
            break;
        case CPU_FEAT_PMU_FEAT_PMUv3p8:
            pmu_string = "v3.8";
            break;
        case CPU_FEAT_PMU_FEAT_PMUv3p9:
            pmu_string = "v3.9";
            break;
    }

    printk(LOGLEVEL_INFO,
           "cpu: features:\n"
           "\table: %s\n"
           "\taderr: %s\n"
           "\tanerr: %s\n"
           "\taie: %s\n"
           "\tb16b16: %s\n"
           "\tclrbhb: %s\n"
           "\tebep: %s\n"
           "\tecbhb: %s\n"
           "\tgcs: %s\n"
           "\thaft: %s\n"
           "\tite: %s\n"
           "\tmec: %s\n"
           "\ts1pie: %s\n"
           "\ts2pie: %s\n"
           "\ts1poe: %s\n"
           "\ts2poe: %s\n"
           "\tpmuv3_icntr: %s\n"
           "\tpmuv3_ss: %s\n"
           "\tprfmslc: %s\n"
           "\tpfar: %s\n"
           "\tsebep: %s\n"
           "\tspmu: %s\n"
           "\tsysinstr128: %s\n"
           "\tsysreg128: %s\n"
           "\tthe: %s\n"
           "\ttrbe_ext: %s\n"
           "\tcmow: %s\n"
           "\thbc: %s\n"
           "\thpmn0: %s\n"
           "\tnmi: %s\n"
           "\tmops: %s\n"
           "\tpacqarma3: %s\n"
           "\trndr_trap: %s\n"
           "\ttidcp1: %s\n"
           "\tafp: %s\n"
           "\thcx: %s\n"
           "\tlpa2: %s\n"
           "\trpres: %s\n"
           "\trme: %s\n"
           "\tsme_fa64: %s\n"
           "\tsme_f64f64: %s\n"
           "\tsme_i16i64: %s\n"
           "\tsme_f16f16: %s\n"
           "\twfxt: %s\n"
           "\txs: %s\n"
           "\tdpb: %s\n"
           "\tadvanced-simd: %s\n"
           "\taes: %s\n"
           "\tsha2: %s\n"
           "\tatomic: %s\n"
           "\tts: %s\n"
           "\ttlb: %s\n"
           "\tpauth: %s\n"
           "\tlrcpc: %s\n"
           "\tfgt: %s\n"
           "\tls64: %s\n"
           "\tfp: %s\n"
           "\tgic: %s\n"
           "\tras: %s\n"
           "\tsve: %s\n"
           "\tsve_aes: %s\n"
           "\tsve_bf16: %s\n"
           "\tamu: %s\n"
           "\tcsv2: %s\n"
           "\tspecres: %s\n"
           "\tssbs: %s\n"
           "\tmte: %s\n"
           "\tsme: %s\n"
           "\tpa_range: %s\n"
           "\tecv: %s\n"
           "\thafdbs: %s\n"
           "\thpds: %s\n"
           "\tpan: %s\n"
           "\tnv: %s\n"
           "\tbbm: %s\n"
           "\tdebug: %s\n"
           "\tpmu: %s\n"
           "\tmtpmu: %s\n"
           "\tbrbe: %s\n"
           "\tbf16: %s\n"
           "\tcsv3: %s\n"
           "\tdgh: %s\n"
           "\tdouble_lock: %s\n"
           "\tets: %s\n"
           "\tsb: %s\n"
           "\tsha1: %s\n"
           "\tcrc32: %s\n"
           "\tntlbpa: %s\n"
           "\tlor: %s\n"
           "\trdm: %s\n"
           "\tvhe: %s\n"
           "\tvmid16: %s\n"
           "\tdot_product: %s\n"
           "\tevt: %s\n"
           "\tfhm: %s\n"
           "\tfp16: %s\n"
           "\ti8mm: %s\n"
           "\tiesb: %s\n"
           "\tlpa: %s\n"
           "\tlsmaoc: %s\n"
           "\tlva: %s\n"
           "\tmpam: %s\n"
           "\tpcsrv8p2: %s\n"
           "\tsha3: %s\n"
           "\tsm3: %s\n"
           "\tsm4: %s\n"
           "\tttcnp: %s\n"
           "\txnx: %s\n"
           "\tuao: %s\n"
           "\tccidx: %s\n"
           "\tfcma: %s\n"
           "\tjscvt: %s\n"
           "\tpacqarma5: %s\n"
           "\tpacimp: %s\n"
           "\tdit: %s\n"
           "\tidst: %s\n"
           "\tlse2: %s\n"
           "\ts2fwb: %s\n"
           "\tsel2: %s\n"
           "\ttrf: %s\n"
           "\tttl: %s\n"
           "\tttst: %s\n"
           "\tbti: %s\n"
           "\texs: %s\n"
           "\te0pd: %s\n"
           "\tfrintts: %s\n"
           "\trndr: %s\n"
           "\ttwed: %s\n"
           "\tete: %s\n"
           "\tsve_pmull128: %s\n"
           "\tsve_bitperm: %s\n"
           "\tsve_sha3: %s\n"
           "\tsve_sm4: %s\n"
           "\ttme: %s\n"
           "\ttrbe: %s\n",
           g_cpu_features.able ? "yes" : "no",
           g_cpu_features.aderr ? "yes" : "no",
           g_cpu_features.anerr ? "yes" : "no",
           g_cpu_features.aie ? "yes" : "no",
           g_cpu_features.b16b16 ? "yes" : "no",
           g_cpu_features.clrbhb ? "yes" : "no",
           g_cpu_features.ebep ? "yes" : "no",
           g_cpu_features.ecbhb ? "yes" : "no",
           g_cpu_features.gcs ? "yes" : "no",
           g_cpu_features.haft ? "yes" : "no",
           g_cpu_features.ite ? "yes" : "no",
           g_cpu_features.mec ? "yes" : "no",
           g_cpu_features.s1pie ? "yes" : "no",
           g_cpu_features.s2pie ? "yes" : "no",
           g_cpu_features.s1poe ? "yes" : "no",
           g_cpu_features.s2poe ? "yes" : "no",
           g_cpu_features.pmuv3_icntr ? "yes" : "no",
           g_cpu_features.pmuv3_ss ? "yes" : "no",
           g_cpu_features.prfmslc ? "yes" : "no",
           g_cpu_features.pfar ? "yes" : "no",
           g_cpu_features.sebep ? "yes" : "no",
           g_cpu_features.spmu ? "yes" : "no",
           g_cpu_features.sysinstr128 ? "yes" : "no",
           g_cpu_features.sysreg128 ? "yes" : "no",
           g_cpu_features.the ? "yes" : "no",
           g_cpu_features.trbe_ext ? "yes" : "no",
           g_cpu_features.cmow ? "yes" : "no",
           g_cpu_features.hbc ? "yes" : "no",
           g_cpu_features.hpmn0 ? "yes" : "no",
           g_cpu_features.nmi ? "yes" : "no",
           g_cpu_features.mops ? "yes" : "no",
           g_cpu_features.pacqarma3 ? "yes" : "no",
           g_cpu_features.rndr_trap ? "yes" : "no",
           g_cpu_features.tidcp1 ? "yes" : "no",
           g_cpu_features.afp ? "yes" : "no",
           g_cpu_features.hcx ? "yes" : "no",
           g_cpu_features.lpa2 ? "yes" : "no",
           g_cpu_features.rpres ? "yes" : "no",
           g_cpu_features.rme ? "yes" : "no",
           g_cpu_features.sme_fa64 ? "yes" : "no",
           g_cpu_features.sme_f64f64 ? "yes" : "no",
           g_cpu_features.sme_i16i64 ? "yes" : "no",
           g_cpu_features.sme_f16f16 ? "yes" : "no",
           g_cpu_features.wfxt ? "yes" : "no",
           g_cpu_features.xs ? "yes" : "no",
           g_cpu_features.dpb == CPU_FEAT_DPB_NONE ?
            "none" :
                g_cpu_features.dpb == CPU_FEAT_DPB2 ?
                    "dpb2" : "dpb",
           g_cpu_features.adv_simd == CPU_FEAT_ADV_SIMD_NONE ?
            "none" :
                g_cpu_features.adv_simd == CPU_FEAT_ADV_SIMD_FULL ?
                    "full" : "partial",
           g_cpu_features.aes == CPU_FEAT_AES_NONE ?
            "none" :
                g_cpu_features.aes == CPU_FEAT_AES ?
                    "aes" : "aes-with-pmull",
           g_cpu_features.sha2 == CPU_FEAT_SHA2_NONE ?
            "none" :
                g_cpu_features.sha2 == CPU_FEAT_SHA256 ?
                    "sha256" : "sha512",
           g_cpu_features.atomic == CPU_FEAT_ATOMIC_NONE ?
            "none" :
                g_cpu_features.atomic == CPU_FEAT_ATOMIC_LSE ?
                    "lse" : "lse128",
           g_cpu_features.ts == CPU_FEAT_NONE ?
            "none" :
                g_cpu_features.ts == CPU_FEAT_TS_FLAG_M ?
                    "flag-m" : "flag-m2",
           g_cpu_features.tlb == CPU_FEAT_TLB_NONE ?
            "none" :
                g_cpu_features.tlb == CPU_FEAT_TLBIOS ?
                    "tlbios" : "tlbirange",
           pauth_string,
           lrcpc_string,
           g_cpu_features.fgt == CPU_FEAT_FGT_NONE ?
            "none" :
                g_cpu_features.fgt == CPU_FEAT_FGT ? "fgt" : "fgt2",
           ls64_string,
           g_cpu_features.fp == CPU_FEAT_FP_NONE ?
            "none" :
                g_cpu_features.fp == CPU_FEAT_FP_PARTIAL ? "partial" : "full",
           g_cpu_features.gic == CPU_FEAT_GIC_NONE ?
            "none" :
                g_cpu_features.gic == CPU_FEAT_GIC_V4 ? "v4" : "v4.1",
           ras_string,
           g_cpu_features.sve == CPU_FEAT_SVE ?
            "sve" :
                g_cpu_features.sve == CPU_FEAT_SVE2 ? "v2" : "v2.1",
           g_cpu_features.sve_aes == CPU_FEAT_SVE_AES_NONE ?
            "none" :
                g_cpu_features.sve_aes == CPU_FEAT_SVE_AES ?
                    "sve-aes" : "sve-aes-with-pmull",
           g_cpu_features.sve_bf16 == CPU_FEAT_SVE_BF16_NONE ?
            "none" :
                g_cpu_features.sve_bf16 == CPU_FEAT_SVE_BF16 ?
                    "bf16" : "ebf16",
           g_cpu_features.amu == CPU_FEAT_AMU_NONE ?
            "none" :
                g_cpu_features.amu == CPU_FEAT_AMU_AMUv1 ?
                    "v1" : "v1.1",
           csv2_string,
           g_cpu_features.specres == CPU_FEAT_NONE ?
            "none" :
                g_cpu_features.specres == CPU_FEAT_SPECRES ?
                    "specres" : "specres2",
           g_cpu_features.ssbs == CPU_FEAT_SSBS_NONE ?
            "none" :
                g_cpu_features.ssbs == CPU_FEAT_SSBS ?
                    "ssbs" : "ssbs2",
           mte_string,
           sme_string,
           parange_string,
           g_cpu_features.ecv == CPU_FEAT_ECV_NONE ?
            "none" :
                g_cpu_features.ecv == CPU_FEAT_ECV_FULL ?
                    "full" : "partial",
           hafdbs_string,
           g_cpu_features.hpds == CPU_FEAT_HPDS_NONE ?
            "none" :
                g_cpu_features.hpds == CPU_FEAT_HPDS ?
                    "hpds" : "hpds2",
           pan_string,
           g_cpu_features.nv == CPU_FEAT_NV_NONE ?
            "none" :
                g_cpu_features.nv == CPU_FEAT_NV ?
                    "nv" : "nv2",
           g_cpu_features.bbm == CPU_FEAT_BBM_LVL0 ?
            "lvl-0" :
                g_cpu_features.bbm == CPU_FEAT_BBM_LVL1 ?
                    "lvl-2" : "lvl-1",
           debug_string,
           pmu_string,
           g_cpu_features.mtpmu == CPU_FEAT_MTPMU ? "mtpmu" : "mtpmu-maybe-3",
           g_cpu_features.brbe == CPU_FEAT_BRBE_NONE ?
            "none" :
                g_cpu_features.brbe == CPU_FEAT_BRBE ?
                    "brbe" : "brbe-v1.1",
           g_cpu_features.bf16 == CPU_FEAT_BF16_NONE ?
            "none" :
                g_cpu_features.bf16 == CPU_FEAT_BF16 ?
                    "bf16" : "ebf16",
           g_cpu_features.csv3 ? "yes" : "no",
           g_cpu_features.dgh ? "yes" : "no",
           g_cpu_features.double_lock ? "yes" : "no",
           g_cpu_features.ets ? "yes" : "no",
           g_cpu_features.sb ? "yes" : "no",
           g_cpu_features.sha1 ? "yes" : "no",
           g_cpu_features.crc32 ? "yes" : "no",
           g_cpu_features.ntlbpa ? "yes" : "no",
           g_cpu_features.lor ? "yes" : "no",
           g_cpu_features.rdm ? "yes" : "no",
           g_cpu_features.vhe ? "yes" : "no",
           g_cpu_features.vmid16 ? "yes" : "no",
           g_cpu_features.dot_product ? "yes" : "no",
           g_cpu_features.evt ? "yes" : "no",
           g_cpu_features.fhm ? "yes" : "no",
           g_cpu_features.fp16 ? "yes" : "no",
           g_cpu_features.i8mm ? "yes" : "no",
           g_cpu_features.iesb ? "yes" : "no",
           g_cpu_features.lpa ? "yes" : "no",
           g_cpu_features.lsmaoc ? "yes" : "no",
           g_cpu_features.lva ? "yes" : "no",
           g_cpu_features.mpam ? "yes" : "no",
           g_cpu_features.pcsrv8p2 ? "yes" : "no",
           g_cpu_features.sha3 ? "yes" : "no",
           g_cpu_features.sm3 ? "yes" : "no",
           g_cpu_features.sm4 ? "yes" : "no",
           g_cpu_features.ttcnp ? "yes" : "no",
           g_cpu_features.xnx ? "yes" : "no",
           g_cpu_features.uao ? "yes" : "no",
           g_cpu_features.ccidx ? "yes" : "no",
           g_cpu_features.fcma ? "yes" : "no",
           g_cpu_features.jscvt ? "yes" : "no",
           g_cpu_features.pacqarma5 ? "yes" : "no",
           g_cpu_features.pacimp ? "yes" : "no",
           g_cpu_features.dit ? "yes" : "no",
           g_cpu_features.idst ? "yes" : "no",
           g_cpu_features.lse2 ? "yes" : "no",
           g_cpu_features.s2fwb ? "yes" : "no",
           g_cpu_features.sel2 ? "yes" : "no",
           g_cpu_features.trf ? "yes" : "no",
           g_cpu_features.ttl ? "yes" : "no",
           g_cpu_features.ttst ? "yes" : "no",
           g_cpu_features.bti ? "yes" : "no",
           g_cpu_features.exs ? "yes" : "no",
           g_cpu_features.e0pd ? "yes" : "no",
           g_cpu_features.frintts ? "yes" : "no",
           g_cpu_features.rndr ? "yes" : "no",
           g_cpu_features.twed ? "yes" : "no",
           g_cpu_features.ete ? "yes" : "no",
           g_cpu_features.sve_pmull128 ? "yes" : "no",
           g_cpu_features.sve_bitperm ? "yes" : "no",
           g_cpu_features.sve_sha3 ? "yes" : "no",
           g_cpu_features.sve_sm4 ? "yes" : "no",
           g_cpu_features.tme ? "yes" : "no",
           g_cpu_features.trbe ? "yes" : "no");
}

void cpu_init() {
    collect_cpu_features();
    print_cpu_features();

    g_base_cpu_info.mpidr = read_mpidr_el1();
    g_base_cpu_info.mpidr &= ~(1ull << 31);

    asm volatile ("msr tpidr_el1, %0" :: "r"(&g_base_cpu_info));
    g_base_cpu_init = true;
}

void
cpu_add_gic_interface(
    const struct acpi_madt_entry_gic_cpu_interface *const intr)
{
    struct cpu_info *cpu = &g_base_cpu_info;
    if (intr->mpidr != g_base_cpu_info.mpidr) {
        cpu = kmalloc(sizeof(struct cpu_info));
        if (cpu == NULL) {
            printk(LOGLEVEL_WARN,
                   "cpu: failed to alloc cpu-info for info created from "
                   "gic-cpu-interface\n");
            return;
        }

        list_init(&cpu->cpu_list);
        list_add(&g_cpu_list, &cpu->cpu_list);
    }

    cpu->spur_int_count = 0;
    cpu->acpi_processor_id = intr->acpi_processor_id;
    cpu->cpu_interface_number = intr->cpu_interface_number;
    cpu->spe_overflow_interrupt = intr->spe_overflow_interrupt;
    cpu->mpidr = intr->mpidr;

    cpu->cpu_interface_region =
        vmap_mmio(RANGE_INIT(intr->phys_base_address, PAGE_SIZE),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    assert_msg(cpu->cpu_interface_region != NULL,
               "cpu: failed to allocate mmio-region for gic-cpu-interface for "
               "cpu with mpidr %" PRIu64 "\n",
               cpu->mpidr);

    cpu->interface = cpu->cpu_interface_region->base;
}