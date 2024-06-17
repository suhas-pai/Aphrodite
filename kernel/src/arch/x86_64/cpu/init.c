/*
 * kernel/src/arch/x86_64/cpu/init.c
 * © suhas pai
 */

#include "asm/cpuid.h"
#include "asm/cr.h"
#include "asm/fsgsbase.h"
#include "asm/msr.h"
#include "asm/xsave.h"

#include "dev/printk.h"
#include "sched/thread.h"
#include "sys/gdt.h"

#include "info.h"

static struct cpu_capabilities g_cpu_capabilities = {
    .supports_avx512 = false,
    .supports_x2apic = false,
    .supports_1gib_pages = false,
    .has_compacted_xsave = false,

    .xsave_user_size = 0,
    .xsave_supervisor_size = 0,

    .xsave_user_features = 0,
    .xsave_supervisor_features = 0,
};

static bool g_base_cpu_init = false;

int16_t g_xsave_feat_noncompacted_offsets[XSAVE_FEAT_MAX] = {
    [0 ... XSAVE_FEAT_MAX - 1] = -1
};

int16_t g_xsave_feat_sizes[XSAVE_FEAT_MAX] = {
    [0 ... XSAVE_FEAT_MAX - 1] = -1
};

uint8_t g_xsave_feat_flags[XSAVE_FEAT_MAX] = {
    [0 ... XSAVE_FEAT_MAX - 1] = 0
};

#define __XSAVE_FEAT_USER_MASK \
    (__XSAVE_FEAT_MASK(XSAVE_FEAT_X87) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_SSE) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_MPX_BNDREGS) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_MPX_BNDCSR) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX_512_OPMASK) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX_512_ZMM_HI256) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX_512_HI16_ZMM))

#define __XSAVE_FEAT_SUPERVISOR_MASK \
    (__XSAVE_FEAT_MASK(XSAVE_FEAT_PASID) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_CET_USER))

// List of features that are disabled in XSTATE. User programs running these
// instructions will trap, and the xstate space will be enlarged.

#define XSAVE_FEAT_XFD_MASK \
    (__XSAVE_FEAT_MASK(XSAVE_FEAT_AVX_512_OPMASK) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX_512_ZMM_HI256) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX_512_HI16_ZMM) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AMX_TILECFG) \
     | __XSAVE_FEAT_MASK(XSAVE_FEAT_AMX_TILEDATA))

static void xsave_init() {
    const xsave_feat_mask_t xsave_supervisor_features =
        __XSAVE_FEAT_MASK(XSAVE_FEAT_X87)
        | __XSAVE_FEAT_MASK(XSAVE_FEAT_SSE)
        | __XSAVE_FEAT_MASK(XSAVE_FEAT_AVX)
        | __XSAVE_FEAT_MASK(XSAVE_FEAT_MPX_BNDREGS)
        | __XSAVE_FEAT_MASK(XSAVE_FEAT_MPX_BNDCSR)
        | __XSAVE_FEAT_MASK(XSAVE_FEAT_CET_USER)
        | __XSAVE_FEAT_MASK(XSAVE_FEAT_CET_SUPERVISOR);

    const xsave_feat_mask_t xsave_user_features =
        g_cpu_capabilities.xsave_user_features & __XSAVE_FEAT_USER_MASK;

    msr_write(IA32_MSR_XFD, XSAVE_FEAT_XFD_MASK);

    xsave_set_supervisor_features(xsave_supervisor_features);
    xsave_set_user_features(xsave_user_features);

    g_xsave_feat_noncompacted_offsets[XSAVE_FEAT_X87] = 0;
    g_xsave_feat_noncompacted_offsets[XSAVE_FEAT_SSE] =
        sizeof(struct xsave_fx_legacy_regs);

    g_xsave_feat_sizes[XSAVE_FEAT_X87] =
        g_xsave_feat_noncompacted_offsets[XSAVE_FEAT_SSE];
    g_xsave_feat_sizes[XSAVE_FEAT_SSE] =
        sizeof_field(struct xsave_fx_regs, xmm);

    const xsave_feat_mask_t all_features_supported =
        g_cpu_capabilities.xsave_supervisor_features
        | g_cpu_capabilities.xsave_user_features;

    printk(LOGLEVEL_INFO,
           "cpu: xsave full feature set is 0x%" XSAVE_FEATURE_MASK_FMT "\n",
           all_features_supported);

    for (enum xsave_feature feat = XSAVE_FEAT_SSE + 1;
         feat <= XSAVE_FEAT_MAX;
         feat++)
    {
        if ((all_features_supported & __XSAVE_FEAT_MASK(feat)) == 0) {
            printk(LOGLEVEL_INFO,
                   "cpu: xsave feat %s is not supported\n",
                   xsave_feat_get_string(feat));
            continue;
        }

        // We can't use cpuid() to get the offset&size pair for supervisor
        // features.

        if (xsave_feat_is_supervisor(feat)) {
            // Record the offset and size as zero, so we know this feature is
            // supported but w/o any information on where its located.

            g_xsave_feat_noncompacted_offsets[feat] = 0;
            g_xsave_feat_sizes[feat] = 0;

            continue;
        }

        uint64_t feat_size, feat_offset, feat_flags, edx;
        cpuid(CPUID_GET_FEATURES_XSAVE,
              /*subleaf=*/(uint64_t)feat,
              /*a=*/&feat_size,
              /*b=*/&feat_offset,
              /*c=*/&feat_flags,
              &edx);

        if (feat_size == 0) {
            printk(LOGLEVEL_INFO,
                   "cpu: xsave feat %s is not supported according to cpuid\n",
                   xsave_feat_get_string(feat));
            continue;
        }

        g_xsave_feat_noncompacted_offsets[feat] = feat_offset;
        g_xsave_feat_sizes[feat] = feat_size;
        g_xsave_feat_flags[feat] = feat_flags;

        printk(LOGLEVEL_INFO,
               "cpu: xsave feat %s has range " RANGE_FMT "\n",
               xsave_feat_get_string(feat),
               RANGE_FMT_ARGS(
                   RANGE_INIT((uint16_t)g_xsave_feat_noncompacted_offsets[feat],
                              (uint16_t)g_xsave_feat_sizes[feat])));
    }
}

static void init_cpuid_features() {
    //
    // Recommended settings for recent x86-64 cpus are:
    //  0 for EM
    //  1 for MP
    //
    // Enable rdgsbase/wrgsbase before the first call to printk() so
    // cpu_in_bad_state() works properly.
    //

    const uint64_t cr0 = read_cr0();
    write_cr0(rm_mask(cr0, __CR0_BIT_EM) | __CR0_BIT_MP);

    const uint64_t cr4 = read_cr4();
    const uint64_t cr4_bits =
        __CR4_BIT_TSD
        | __CR4_BIT_DE
        | __CR4_BIT_PGE
        | __CR4_BIT_OSFXSR
        | __CR4_BIT_OSXMMEXCPTO
        | __CR4_BIT_FSGSBASE
        | __CR4_BIT_SMEP
        | __CR4_BIT_SMAP
        | __CR4_BIT_OSXSAVE;

    write_cr4(cr4 | cr4_bits);
    printk(LOGLEVEL_INFO,
           "cpu: control-registers:\n"
           "\tcr0: 0x%" PRIx64 "\n"
           "\tcr4: 0x%" PRIx64 "\n",
           cr0,
           cr4);

    {
        uint64_t eax, ebx, ecx, edx;
        cpuid(CPUID_GET_FEATURES, /*subleaf=*/0, &eax, &ebx, &ecx, &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: cpuid_get_features: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_ecx_features =
            __CPUID_FEAT_ECX_SSE3
            | __CPUID_FEAT_ECX_SSSE3
            | __CPUID_FEAT_ECX_SSE4_1
            | __CPUID_FEAT_ECX_SSE4_2
            | __CPUID_FEAT_ECX_POPCNT
            | __CPUID_FEAT_ECX_XSAVE
            | __CPUID_FEAT_ECX_RDRAND;
        const uint32_t expected_edx_features =
            __CPUID_FEAT_EDX_FPU
            | __CPUID_FEAT_EDX_DE
            | __CPUID_FEAT_EDX_PSE
            | __CPUID_FEAT_EDX_TSC
            | __CPUID_FEAT_EDX_MSR
            | __CPUID_FEAT_EDX_PAE
            | __CPUID_FEAT_EDX_APIC
            | __CPUID_FEAT_EDX_SEP
            | __CPUID_FEAT_EDX_MTRR
            | __CPUID_FEAT_EDX_CMOV
            | __CPUID_FEAT_EDX_PAT
            | __CPUID_FEAT_EDX_PSE36
            | __CPUID_FEAT_EDX_PGE
            | __CPUID_FEAT_EDX_SSE
            | __CPUID_FEAT_EDX_SSE2;

        assert((ecx & expected_ecx_features) == expected_ecx_features);
        assert((edx & expected_edx_features) == expected_edx_features);

        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_x2apic = ecx & __CPUID_FEAT_ECX_X2APIC;
        }
    }
    {
        uint64_t eax, ebx, ecx = 0, edx;
        cpuid(CPUID_GET_FEATURES_EXTENDED_7,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_features_extended_7(ecx=0): "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_ebx_features =
            __CPUID_FEAT_EXT7_ECX0_EBX_FSGSBASE
            | __CPUID_FEAT_EXT7_ECX0_EBX_BMI1
            | __CPUID_FEAT_EXT7_ECX0_EBX_BMI2
            | __CPUID_FEAT_EXT7_ECX0_EBX_REP_MOVSB_STOSB
            | __CPUID_FEAT_EXT7_ECX0_EBX_SMAP;

        assert((ebx & expected_ebx_features) == expected_ebx_features);
        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_avx512 =
                ebx & __CPUID_FEAT_EXT7_ECX0_EBX_AVX512F;
        }
    }
    {
        uint64_t eax, ebx, ecx = 0, edx;
        cpuid(CPUID_GET_POWER_MANAGEMENT_INSTR,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_power_management_instr: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        if ((eax & __CPUID_FEAT_PWR_MGMT_EAX_APIC_TIMER_ALWAYS_RUNNING) == 0) {
            panic("cpu: doesn't support always running lapic timer\n");
        }
    }
    {
        uint64_t eax, ebx, ecx = 1, edx;
        cpuid(CPUID_GET_FEATURES_EXTENDED_7,
              /*subleaf=*/1,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_features_extended_7(ecx=1): "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax, ebx, ecx, edx);

        const uint32_t expected_eax_features =
            __CPUID_FEAT_EXT7_ECX1_EAX_FAST_ZEROLEN_REP_MOVSB
            | __CPUID_FEAT_EXT7_ECX1_EAX_FAST_ZEROLEN_REP_STOSB
            | __CPUID_FEAT_EXT7_ECX1_EAX_FAST_SHORT_REP_CMPSB_SCASB;

        assert((eax & expected_eax_features) == expected_eax_features);
    }
    {
        uint64_t eax, ebx, ecx = 0, edx;
        cpuid(CPUID_GET_LARGEST_EXTENDED_FUNCTION | CPUID_GET_FEATURES,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: get_features_extended_0x800000007: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_edx_features =
            __CPUID_FEAT_EXT80000001_EDX_SYSCALL_SYSRET;

        assert((edx & expected_edx_features) == expected_edx_features);
        if (!g_base_cpu_init) {
            g_cpu_capabilities.supports_1gib_pages =
                edx & __CPUID_FEAT_EXT80000001_EDX_1GIB_PAGES;
            largepage_level_info_list[LARGEPAGE_LEVEL_1GIB - 1].is_supported =
                g_cpu_capabilities.supports_1gib_pages;

            if (g_cpu_capabilities.supports_1gib_pages) {
                printk(LOGLEVEL_INFO, "cpu: supports 1gib pages\n");
            } else {
                printk(LOGLEVEL_INFO, "cpu: does NOT support 1gib pages\n");
            }
        }
    }
    {
        uint64_t eax, ebx, ecx, edx;
        cpuid(CPUID_GET_FEATURES_XSAVE,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: cpuid_get_features_xsave: "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        g_cpu_capabilities.xsave_user_features = (uint64_t)edx << 32 | eax;
        g_cpu_capabilities.xsave_user_size = ecx;

        printk(LOGLEVEL_INFO,
               "cpu: xsave user size is %" PRIu16 "\n",
               g_cpu_capabilities.xsave_user_size);
    }
    {
        uint64_t eax, ebx, ecx = 1, edx;
        cpuid(CPUID_GET_FEATURES_XSAVE,
              /*subleaf=*/0,
              &eax,
              &ebx,
              &ecx,
              &edx);

        printk(LOGLEVEL_INFO,
               "cpuid: cpuid_get_features_xsave(ecx=1): "
               "eax: 0x%" PRIX64 " "
               "ebx: 0x%" PRIX64 " "
               "ecx: 0x%" PRIX64 " "
               "edx: 0x%" PRIX64 "\n",
               eax,
               ebx,
               ecx,
               edx);

        const uint32_t expected_eax_features =
            __CPUID_FEAT_XSAVE_ECX1_EAX_SUPPORTS_XSAVEOPT
            | __CPUID_FEAT_XSAVE_ECX1_EAX_SUPPORTS_XSAVE_COMPACTED
            | __CPUID_FEAT_XSAVE_ECX1_EAX_SUPPORTS_XGETBV
            | __CPUID_FEAT_XSAVE_ECX1_EAX_SUPPORTS_XSAVES_XSTORS
            | __CPUID_FEAT_XSAVE_ECX1_EAX_SUPPORTS_XFD;

        assert((eax & expected_eax_features) == expected_eax_features);

        g_cpu_capabilities.xsave_supervisor_size = ebx;
        g_cpu_capabilities.xsave_supervisor_features =
            (uint64_t)edx << 32 | ecx;

        printk(LOGLEVEL_INFO,
               "cpu: xsave supervisor size is %" PRIu16 "\n",
               g_cpu_capabilities.xsave_supervisor_size);
    }

    // Enable Syscalls
    msr_write(IA32_MSR_EFER,
              msr_read(IA32_MSR_EFER) | __IA32_MSR_EFER_BIT_SCE);

    // Setup Syscall MSRs
    msr_write(IA32_MSR_STAR,
              ((uint64_t)gdt_get_kernel_code_segment() << 32
               | (uint64_t)gdt_get_user_data_segment() << 48));

    msr_write(IA32_MSR_FMASK, /*value=*/0);
    msr_write(IA32_MSR_MISC_ENABLE,
              (msr_read(IA32_MSR_MISC_ENABLE)
               | __IA32_MSR_MISC_FAST_STRING_ENABLE));

    xsave_init();
    printk(LOGLEVEL_INFO,
           "cpu: xsave compacted size is %" PRIu16 " bytes\n",
           xsave_get_compacted_size());
}

__optimize(3) const struct cpu_capabilities *get_cpu_capabilities() {
    return &g_cpu_capabilities;
}

__optimize(3) void cpu_early_init() {
    write_gsbase_early((uint64_t)&kernel_main_thread);
    msr_write(IA32_MSR_KERNEL_GS_BASE, (uint64_t)&kernel_main_thread);

    list_add(&kernel_process.pagemap.cpu_list, &g_base_cpu_info.pagemap_node);
    init_cpuid_features();

    g_base_cpu_init = true;
}

void cpu_init() {

}