/*
 * kernel/src/arch/x86_64/asm/xsave.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

#include "asm/msr.h"
#include "asm/xcr.h"

#include "lib/assert.h"
#include "lib/inttypes.h"

enum xsave_header_xcompbv_flags {
    __XSAVE_XCOMPBV_USES_COMPACTED_FORM = 1ull << 63,
};

struct xsave_header {
    uint64_t xstate_bv;
    uint64_t xcomp_bv;

    char reserved[48];
};

struct xsave_fx_legacy_regs {
    /*
     * Bytes 1:0, 3:2, 7:6. These are used for the x87 FPU Control Word (FCW),
     * the x87 FPU Status Word (FSW), and the x87 FPU Opcode (FOP),
     * respectively.
     */
    uint16_t control;
    uint16_t status;

    /*
     * Byte 4 is used for an abridged version of the x87 FPU Tag Word (FTW). The
     * following items describe its usage:
     *   — For each j, 0 ≤ j ≤ 7, XSAVE, XSAVEOPT, XSAVEC, and XSAVES save a 0
     *     into bit j of byte 4 if x87 FPU data register STj has a empty tag;
     *     otherwise, XSAVE, XSAVEOPT, XSAVEC, and XSAVES save a 1 into bit j of
     *     byte 4.
     *   — For each j, 0 ≤ j ≤ 7, XRSTOR and XRSTORS establish the tag value for
     *     x87 FPU data register STj as follows. If bit j of byte 4 is 0, the
     *     tag for STj in the tag register for that data register is marked
     *     empty (11B); otherwise, the x87 FPU sets the tag for STj based on the
     *     value being loaded into that register (see below).
     */
    uint16_t tag;
    uint16_t opcode;

    union {
        struct {
            uint32_t eip;
            uint32_t cs;
            uint32_t fop;
            uint32_t fos;
        };

        struct {
            uint64_t rip;
            uint64_t rdp;
        };
    };

    uint32_t mxcsr;
    uint32_t mxcsr_mask;
    uint16_t st0[8][8];
} __packed;

struct xsave_fx_regs {
    struct xsave_fx_legacy_regs legacy;
    uint16_t xmm[8][16];

    char reserved[96];
} __packed;

struct xsave_uint128_register {
    uint64_t low64;
    uint64_t high64;
};

struct xsave_avx_state {
    struct xsave_fx_regs fx;
    struct xsave_header header;
    struct xsave_uint128_register ymm[16];
} __packed;

struct xsave_uint256_register {
    struct xsave_uint128_register low128;
    struct xsave_uint128_register high128;
};

struct xsave_uint512_register {
    struct xsave_uint256_register low256;
    struct xsave_uint256_register high256;
};

struct xsave_avx512_opmask_state {
    uint64_t opmask[8];
} __packed;

struct xsave_avx512_zmm_high_state {
    struct xsave_uint256_register zmm_high[16];
} __packed;

struct xsave_avx512_hi16_state {
    struct xsave_uint512_register hi16_zmm[16];
} __packed;

struct xsave_avx512_state {
    struct xsave_avx512_opmask_state opmask_state;
    struct xsave_avx512_zmm_high_state zmm_high[16];
    struct xsave_avx512_hi16_state hi16_zmm_state[16];
} __packed;

struct xsave_pkru_state {
    uint32_t pkru;
} __packed;

struct xsave_cet_user_state {
    uint32_t cet_user;
    uint32_t cet_ssp;
} __packed;

struct xsave_lbr_entry {
    uint64_t from;
    uint64_t to;
    uint64_t info;
} __packed;

struct xsave_lbr_state {
    uint64_t control;
    uint64_t depth;
    uint64_t from;
    uint64_t to;
    uint64_t info;

    struct xsave_lbr_entry entries[];
} __packed;

struct xsave_pasid_state {
    uint64_t pasid;
} __packed;

enum xsave_feature {
    XSAVE_FEAT_X87,
    XSAVE_FEAT_SSE,
    XSAVE_FEAT_AVX,
    XSAVE_FEAT_MPX_BNDREGS,
    XSAVE_FEAT_MPX_BNDCSR,
    XSAVE_FEAT_AVX_512_OPMASK,
    XSAVE_FEAT_AVX_512_ZMM_HI256,
    XSAVE_FEAT_AVX_512_HI16_ZMM,
    XSAVE_FEAT_PROC_TRACE,
    XSAVE_FEAT_PKRU,

    // Supervisor only
    XSAVE_FEAT_PASID,
    XSAVE_FEAT_CET_USER,
    XSAVE_FEAT_CET_SUPERVISOR,
    XSAVE_FEAT_HDC,
    XSAVE_FEAT_UINTR,
    XSAVE_FEAT_LBR,
    XSAVE_FEAT_HWP,

    // User only
    XSAVE_FEAT_AMX_TILECFG,
    XSAVE_FEAT_AMX_TILEDATA,

    XSAVE_FEAT_MAX = XSAVE_FEAT_AMX_TILEDATA
};

enum xsave_feature_flags {
    __XSAVE_FEAT_FLAG_SUPERVISOR_FEAT = 1 << 0,
    __XSAVE_FEAT_FLAG_64B_ALIGNED = 1 << 1,
};

typedef uint32_t xsave_feat_mask_t;

#define __XSAVE_FEAT_MASK(feat) (1ull << (feat))
#define XSAVE_FEATURE_MASK_FMT PRIx32

__debug_optimize(3)
static inline void xsave_feat_disable(const enum xsave_feature feat) {
    msr_write(IA32_MSR_XFD, msr_read(IA32_MSR_XFD) | __XSAVE_FEAT_MASK(feat));
}

__debug_optimize(3)
static inline void xsave_feat_enable(const enum xsave_feature feat) {
    msr_write(IA32_MSR_XFD,
              rm_mask(msr_read(IA32_MSR_XFD), __XSAVE_FEAT_MASK(feat)));
}

bool xsave_feat_is_supervisor(enum xsave_feature feat);
bool xsave_feat_has_aligned_offset(enum xsave_feature feat);

int16_t xsave_feat_get_offset(uint64_t xcmo_bv, const enum xsave_feature feat);
uint16_t xsave_get_compacted_size();

__debug_optimize(3)
static inline const char *xsave_feat_get_string(const enum xsave_feature feat) {
    switch (feat) {
        case XSAVE_FEAT_X87:
            return "x87";
        case XSAVE_FEAT_SSE:
            return "sse";
        case XSAVE_FEAT_AVX:
            return "avx";
        case XSAVE_FEAT_MPX_BNDREGS:
            return "bndregs";
        case XSAVE_FEAT_MPX_BNDCSR:
            return "bndcsr";
        case XSAVE_FEAT_AVX_512_OPMASK:
            return "opmask";
        case XSAVE_FEAT_AVX_512_ZMM_HI256:
            return "avx512-zmm-hi256";
        case XSAVE_FEAT_AVX_512_HI16_ZMM:
            return "avx512-hi16-zmm";
        case XSAVE_FEAT_PROC_TRACE:
            return "proc-trace";
        case XSAVE_FEAT_PKRU:
            return "pkru";
        case XSAVE_FEAT_PASID:
            return "pasid";
        case XSAVE_FEAT_CET_USER:
            return "cet-user";
        case XSAVE_FEAT_CET_SUPERVISOR:
            return "cet-supervisor";
        case XSAVE_FEAT_HDC:
            return "hdc";
        case XSAVE_FEAT_UINTR:
            return "uintr";
        case XSAVE_FEAT_LBR:
            return "lbr";
        case XSAVE_FEAT_HWP:
            return "hwp";
        case XSAVE_FEAT_AMX_TILECFG:
            return "amx-tilecfg";
        case XSAVE_FEAT_AMX_TILEDATA:
            return "amx-tiledata";
    }

    verify_not_reached();
}

__debug_optimize(3)
static inline void xsave_set_supervisor_features(const uint32_t features) {
    msr_write(IA32_MSR_XSS, msr_read(IA32_MSR_XSS) | features);
}

__debug_optimize(3)
static inline void xsave_set_user_features(const uint32_t features) {
    const uint64_t xcr = read_xcr(XCR_XSTATE_FEATURES_ENABLED);
    write_xcr(XCR_XSTATE_FEATURES_ENABLED, xcr | features);
}

__debug_optimize(3) static inline void xsave_user_into(void *const buffer) {
    asm volatile ("xsave (%0)" :: "a"(buffer));
}

__debug_optimize(3)
static inline void xsave_supervisor_into(void *const buffer) {
    asm volatile ("xsaves (%0)" :: "a"(buffer));
}

__debug_optimize(3)
static inline void xrstor_user_from(const void *const buffer) {
    asm volatile ("xrstor (%0)" :: "a"(buffer));
}

__debug_optimize(3)
static inline void xrstor_supervisor_from(const void *const buffer) {
    asm volatile ("xrstors (%0)" :: "a"(buffer));
}