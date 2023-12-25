/*
 * kernel/src/acpi/init.c
 * © suhas pai
 */

#if defined(__aarch64__)
    #include "acpi/gtdt.h"
#endif /* defined(__aarch64__)*/

#include "acpi/mcfg.h"

#include "dev/printk.h"
#include "mm/mm_types.h"
#include "sys/boot.h"

#include "api.h"
#include "fadt.h"
#include "madt.h"
#include "pptt.h"
#include "spcr.h"

static struct acpi_info g_info = {
    .madt = NULL,
    .fadt = NULL,
    .mcfg = NULL,
    .rsdp = NULL,
    .rsdt = NULL,

#if defined(__aarch64__)
    .msi_frame_list = ARRAY_INIT(sizeof(struct acpi_msi_frame)),
#endif /* defined(__aarch64__) */

    .iso_list = ARRAY_INIT(sizeof(struct apic_iso_info)),
    .nmi_lint = 0,

#if defined(__x86_64__)
    .using_x2apic = false
#endif /* defined(__x86_64__) */
};

__optimize(3) static inline bool has_xsdt() {
    return g_info.rsdp->revision >= 2 && g_info.rsdp->v2.xsdt_addr != 0;
}

__optimize(3)
static inline void acpi_recurse(void (*callback)(const struct acpi_sdt *)) {
    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint64_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            callback(sdt);
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint32_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            callback(sdt);
        }
    }
}

__optimize(3)
static inline void acpi_init_each_sdt(const struct acpi_sdt *const sdt) {
    printk(LOGLEVEL_INFO,
           "acpi: found sdt \"" SV_FMT "\"\n",
           SV_FMT_ARGS(
            sv_create_nocheck(sdt->signature, sizeof(sdt->signature))));

    if (memcmp(sdt->signature, "APIC", sizeof(sdt->signature)) == 0) {
        g_info.madt = (const struct acpi_madt *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "FACP", sizeof(sdt->signature)) == 0) {
        g_info.fadt = (const struct acpi_fadt *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "MCFG", sizeof(sdt->signature)) == 0) {
        g_info.mcfg = (const struct acpi_mcfg *)sdt;
        return;
    }

    if (memcmp(sdt->signature, "PPTT", sizeof(sdt->signature)) == 0) {
        g_info.pptt = (const struct acpi_pptt *)sdt;
        return;
    }

#if defined(__aarch64__)
    if (memcmp(sdt->signature, "GTDT", sizeof(sdt->signature)) == 0) {
        g_info.gtdt = (const struct acpi_gtdt *)sdt;
        return;
    }
#endif /* defined(__aarch64__) */
}

void acpi_parse_tables() {
    g_info.rsdp = boot_get_rsdp();
    if (g_info.rsdp == NULL) {
        printk(LOGLEVEL_WARN, "acpi: tables are missing\n");
        return;
    }

    const uint64_t oem_id_length =
        strnlen(g_info.rsdp->oem_id, sizeof(g_info.rsdp->oem_id));
    const struct string_view oem_id =
        sv_create_nocheck(g_info.rsdp->oem_id, oem_id_length);

    printk(LOGLEVEL_INFO, "acpi: oem is \"" SV_FMT "\"\n", SV_FMT_ARGS(oem_id));
    if (has_xsdt()) {
        g_info.rsdt = phys_to_virt(g_info.rsdp->v2.xsdt_addr);
    } else {
        g_info.rsdt = phys_to_virt(g_info.rsdp->rsdt_addr);
    }

    if (g_info.rsdt->sdt.length < sizeof(struct acpi_sdt)) {
        printk(LOGLEVEL_WARN, "acpi: table-length is too short\n");
        return;
    }

    printk(LOGLEVEL_INFO,
           "acpi: revision: %" PRIu8 "\n",
           g_info.rsdp->revision);

    printk(LOGLEVEL_INFO, "acpi: uses xsdt? %s\n", has_xsdt() ? "yes" : "no");
    printk(LOGLEVEL_INFO, "acpi: rsdt at %p\n", g_info.rsdt);

    acpi_recurse(acpi_init_each_sdt);
}

void acpi_init(void) {
    if (get_acpi_info()->madt != NULL) {
        madt_init(get_acpi_info()->madt);
    }

    if (get_acpi_info()->fadt != NULL) {
        fadt_init(get_acpi_info()->fadt);
    }

#if defined(__aarch64__)
    if (get_acpi_info()->gtdt != NULL) {
        gtdt_init(get_acpi_info()->gtdt);
    }
#endif /* defined(__aarch64__) */

    if (get_acpi_info()->pptt != NULL) {
        pptt_init(get_acpi_info()->pptt);
    }

    const struct acpi_sdt *const spcr_sdt = acpi_lookup_sdt("SPCR");
    if (spcr_sdt != NULL) {
        const struct acpi_spcr *const spcr = (const struct acpi_spcr *)spcr_sdt;
        spcr_init(spcr);
    }

    if (get_acpi_info()->mcfg != NULL) {
        mcfg_init(get_acpi_info()->mcfg);
    }
}

__optimize(3)
const struct acpi_sdt *acpi_lookup_sdt(const char signature[static const 4]) {
    if (get_acpi_info()->rsdp == NULL) {
        return NULL;
    }

    if (has_xsdt()) {
        uint64_t *const data = (uint64_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint64_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            if (memcmp(sdt->signature,
                       signature,
                       sizeof(sdt->signature)) == 0)
            {
                return sdt;
            }
        }
    } else {
        uint32_t *const data = (uint32_t *)(uint64_t)g_info.rsdt->ptrs;
        const uint32_t entry_count =
            (g_info.rsdt->sdt.length - sizeof(struct acpi_sdt)) /
            sizeof(uint32_t);

        for (uint32_t i = 0; i != entry_count; i++) {
            struct acpi_sdt *const sdt = phys_to_virt(data[i]);
            if (memcmp(sdt->signature,
                       signature,
                       sizeof(sdt->signature)) == 0)
            {
                return sdt;
            }
        }
    }

    printk(LOGLEVEL_WARN,
           "acpi: failed to find entry with signature \"" SV_FMT "\"\n",
           SV_FMT_ARGS(sv_create_nocheck(signature, 4)));
    return NULL;
}

__optimize(3) const struct acpi_info *get_acpi_info() {
    return &g_info;
}

__optimize(3) struct acpi_info *get_acpi_info_mut() {
    return &g_info;
}