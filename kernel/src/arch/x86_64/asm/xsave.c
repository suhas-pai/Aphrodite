/*
 * kernel/src/arch/x86_64/asm/xsave.c
 * © suhas pai
 */

#include "asm/cpuid.h"
#include "lib/align.h"

#include "xsave.h"

extern int16_t g_xsave_feat_noncompacted_offsets[XSAVE_FEAT_MAX];
extern int16_t g_xsave_feat_sizes[XSAVE_FEAT_MAX];
extern uint8_t g_xsave_feat_flags[XSAVE_FEAT_MAX];

__optimize(3) bool xsave_feat_is_supervisor(const enum xsave_feature feat) {
    return g_xsave_feat_flags[feat] & __XSAVE_FEAT_FLAG_SUPERVISOR_FEAT;
}

__optimize(3)
bool xsave_feat_has_aligned_offset(const enum xsave_feature feat) {
    return g_xsave_feat_flags[feat] & __XSAVE_FEAT_FLAG_64B_ALIGNED;
}

__optimize(3) int16_t
xsave_feat_get_offset(const uint64_t xcomp_bv, const enum xsave_feature feat) {
    const bool has_compacted_xsave =
        xcomp_bv & __XSAVE_XCOMPBV_USES_COMPACTED_FORM;

    if (!has_compacted_xsave || feat <= XSAVE_FEAT_SSE) {
        return g_xsave_feat_noncompacted_offsets[feat];
    }

    int16_t offset = sizeof(struct xsave_fx_regs);
    for (enum xsave_feature iter = XSAVE_FEAT_SSE + 1; iter <= feat; iter++) {
        if (xsave_feat_has_aligned_offset(feat)) {
            offset = align_up_assert((uint64_t)offset, /*boundary=*/64);
        }

        offset += g_xsave_feat_sizes[feat];
    }

    return offset;
}

__optimize(3) uint16_t xsave_get_compacted_size() {
    uint64_t eax, ebx, ecx, edx;
    cpuid(CPUID_GET_FEATURES_XSAVE, /*subleaf=*/1, &eax, &ebx, &ecx, &edx);

    return ebx;
}
