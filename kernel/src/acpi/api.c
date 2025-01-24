/*
 * kernel/src/acpi/init.c
 * © suhas pai
 */

#ifdef USE_UACPI
    #include <uacpi/event.h>
    #include <uacpi/tables.h>
    #include <uacpi/uacpi.h>
#endif /* defined(USE_UACPI) */

#if defined(__aarch64__)
    #include "acpi/gtdt.h"
#endif /* defined(__aarch64__)*/

#include "acpi/api.h"

#include "acpi/fadt.h"
#include "acpi/madt.h"
#include "acpi/mcfg.h"
#include "acpi/pptt.h"
#include "acpi/spcr.h"

#include "dev/printk.h"
#include "mm/mm_types.h"
#include "sys/boot.h"

static struct acpi_info g_info = {
    .madt = nullptr,
    .fadt = nullptr,
    .rsdp = nullptr,

#ifndef USE_UACPI
    .mcfg = nullptr,
    .rsdt = nullptr,
#endif /* !defined(USE_UACPI) */

#if defined(__aarch64__)
    .msi_frame_list = ARRAY_INIT(sizeof(struct acpi_msi_frame)),
#endif /* defined(__aarch64__) */

    .iso_list = ARRAY_INIT(sizeof(struct apic_iso_info)),
    .nmi_lint = 0,
};

__debug_optimize(3) const struct acpi_info *get_acpi_info() {
    return &g_info;
}

__debug_optimize(3) struct acpi_info *get_acpi_info_mut() {
    return &g_info;
}

__debug_optimize(3) static inline bool has_xsdt() {
    return g_info.rsdp->revision >= 2 && g_info.rsdp->v2.xsdt_addr != 0;
}

__debug_optimize(3)
const struct os_acpi_sdt *acpi_lookup_sdt(const char sig[static const 4]) {
    if (get_acpi_info()->rsdp == nullptr) {
        return nullptr;
    }

    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct os_acpi_sdt)) /
            sizeof(uint64_t);

        ptrarr_foreach(data, entry_count, entry) {
            if (*entry == 0) {
                continue;
            }

            struct os_acpi_sdt *const sdt = phys_to_virt(*entry);
            if (memcmp(sdt->signature, sig, sizeof(sdt->signature)) == 0) {
                return sdt;
            }
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct os_acpi_sdt)) /
            sizeof(uint32_t);

        ptrarr_foreach(data, entry_count, entry) {
            if (*entry == 0) {
                continue;
            }

            struct os_acpi_sdt *const sdt = phys_to_virt(*entry);
            if (memcmp(sdt->signature, sig, sizeof(sdt->signature)) == 0) {
                return sdt;
            }
        }
    }

    printk(LOGLEVEL_WARN,
            "acpi: failed to find entry with signature \"" SV_FMT "\"\n",
            SV_FMT_ARGS(sv_create_nocheck(sig, 4)));

    return nullptr;
}

__debug_optimize(3)
static inline void acpi_recurse(void (*callback)(const struct os_acpi_sdt *)) {
    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct os_acpi_sdt)) /
            sizeof(uint64_t);

        ptrarr_foreach(data, entry_count, entry) {
            if (*entry == 0) {
                continue;
            }

            struct os_acpi_sdt *const sdt = phys_to_virt(*entry);
            callback(sdt);
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct os_acpi_sdt)) /
            sizeof(uint32_t);

        ptrarr_foreach(data, entry_count, entry) {
            if (*entry == 0) {
                continue;
            }

            struct os_acpi_sdt *const sdt = phys_to_virt(*entry);
            callback(sdt);
        }
    }
}

__debug_optimize(3)
static inline void acpi_init_each_sdt(const struct os_acpi_sdt *const sdt) {
    const struct string_view signature_sv =
        sv_create_nocheck(sdt->signature, sizeof(sdt->signature));

    printk(LOGLEVEL_INFO,
           "acpi: found sdt \"" SV_FMT "\"\n",
           SV_FMT_ARGS(signature_sv));

    if (memcmp(sdt->signature, "APIC", sizeof(sdt->signature)) == 0) {
        g_info.madt = (const struct os_acpi_madt *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "FACP", sizeof(sdt->signature)) == 0) {
        g_info.fadt = (const struct os_acpi_fadt *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "MCFG", sizeof(sdt->signature)) == 0) {
        g_info.mcfg = (const struct os_acpi_mcfg *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "PPTT", sizeof(sdt->signature)) == 0) {
        g_info.pptt = (const struct os_acpi_pptt *)sdt;
        return;
    }

#if defined(__aarch64__)
    if (memcmp(sdt->signature, "GTDT", sizeof(sdt->signature)) == 0) {
        g_info.gtdt = (const struct os_acpi_gtdt *)sdt;
        return;
    }
#endif /* defined(__aarch64__) */
}

__debug_optimize(3) static inline
void acpi_print_each_sdt(const struct os_acpi_sdt *const sdt) {
    printk(LOGLEVEL_INFO,
           "acpi: found sdt \"" SV_FMT "\"\n",
           SV_FMT_ARGS(
               sv_create_nocheck(sdt->signature, sizeof(sdt->signature))));
}

void acpi_init(void) {
    g_info.rsdp = boot_get_rsdp();
    if (g_info.rsdp == nullptr) {
        printk(LOGLEVEL_WARN, "acpi: tables are missing\n");
        return;
    }

    if (has_xsdt()) {
        g_info.rsdt = phys_to_virt(g_info.rsdp->v2.xsdt_addr);
    } else {
        g_info.rsdt = phys_to_virt(g_info.rsdp->rsdt_addr);
    }

    if (g_info.rsdt->sdt.length < sizeof(struct os_acpi_sdt)) {
        printk(LOGLEVEL_WARN, "acpi: table-length is too short\n");
        return;
    }

    acpi_recurse(acpi_init_each_sdt);

    const __auto_type oem_id_length =
        strnlen(g_info.rsdp->oem_id, sizeof(g_info.rsdp->oem_id));
    const __auto_type oem_id =
        sv_create_nocheck(g_info.rsdp->oem_id, oem_id_length);

    printk(LOGLEVEL_INFO,
           "acpi:\n"
           "\t\t" "oem is \"" SV_FMT "\"\n"
           "\t\t" "revision: %" PRIu8 "\n"
           "\t\t" "uses xsdt? %s\n"
           "\t\t" "rsdt at %p\n",
           SV_FMT_ARGS(oem_id),
           g_info.rsdp->revision,
           has_xsdt() ? "yes" : "no",
           g_info.rsdt);

    acpi_recurse(acpi_print_each_sdt);

    const struct os_acpi_sdt *const spcr = acpi_lookup_sdt("SPCR");
    if (spcr != nullptr) {
        spcr_init((const struct os_acpi_spcr *)spcr);
    }

    if (get_acpi_info()->madt != nullptr) {
        madt_init(get_acpi_info()->madt);
    }

    if (get_acpi_info()->fadt != nullptr) {
        fadt_init(get_acpi_info()->fadt);
    }

#if defined(__aarch64__)
    if (get_acpi_info()->gtdt != nullptr) {
        gtdt_init(get_acpi_info()->gtdt);
    }
#endif /* defined(__aarch64__) */

    if (get_acpi_info()->pptt != nullptr) {
        pptt_init(get_acpi_info()->pptt);
    }

    if (get_acpi_info()->mcfg != nullptr) {
        mcfg_init(get_acpi_info()->mcfg);
    }

#ifdef USE_UACPI
    /*
     * Start with this as the first step of the initialization. This loads
     * all tables, brings the event subsystem online, and enters ACPI mode.
     * We pass in 0 as the flags as we don't want to override any default
     * behavior for now.
     */

    uacpi_status ret = uacpi_initialize(0);
    if (uacpi_unlikely_error(ret)) {
        printk(LOGLEVEL_WARN,
               "uacpi_initialize error: %s\n",
               uacpi_status_to_string(ret));
        return;
    }

    /*
     * Load the AML namespace. This feeds DSDT and all SSDTs to the
     * interpreter for execution.
     */

    ret = uacpi_namespace_load();
    if (uacpi_unlikely_error(ret)) {
        printk(LOGLEVEL_WARN,
               "uacpi_namespace_load error: %s\n",
               uacpi_status_to_string(ret));
        return;
    }

    /*
     * Initialize the namespace. This calls all necessary _STA/_INI AML
     * methods, as well as _REG for registered operation region handlers.
     */

    ret = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(ret)) {
        printk(LOGLEVEL_WARN,
               "uacpi_namespace_initialize error: %s\n",
               uacpi_status_to_string(ret));
        return;
    }

    /*
     * Tell uACPI that we have marked all GPEs we wanted for wake (even
     * though we haven't actually marked any, as we have no power management
     * support right now). This is needed to let uACPI enable all unmarked
     * GPEs that have a corresponding AML handler. These handlers are used
     * by the firmware to dynamically execute AML code at runtime to e.g.
     * react to thermal events or device hotplug.
     */

    ret = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(ret)) {
        printk(LOGLEVEL_WARN,
               "uACPI GPE initialization error: %s\n",
               uacpi_status_to_string(ret));
        return;
    }

    /*
     * That's it, uACPI is now fully initialized and working! You can
     * proceed to using any public API at your discretion. The next
     * recommended step is namespace enumeration and device discovery so you
     * can bind drivers to ACPI objects.
     */
#endif
}
