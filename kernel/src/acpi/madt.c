/*
 * kernel/src/acpi/madt.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "acpi/structs.h"

#if __has_include("acpi/extra_structs.h")
    #include "acpi/extra_structs.h"
#endif /* __has_include("acpi/extra_structs.h") */

#if defined(__x86_64__)
    #include "apic/ioapic.h"
    #include "apic/lapic.h"
    #include "apic/init.h"
#endif /* defined(__x86_64__) */

#include "dev/printk.h"

#if defined(__aarch64__)
    #include "sys/gic/api.h"
    #include "sys/gic/its.h"

    #include "sys/gic/v2.h"
    #include "sys/gic/v3.h"
#endif /* defined(__aarch64__) */

void madt_init(const struct acpi_madt *const madt) {
    const struct acpi_madt_entry_header *iter = NULL;

#if defined(__x86_64__)
    uint64_t local_apic_base = madt->local_apic_base;
#elif defined(__aarch64__)
    const struct acpi_madt_entry_gic_distributor *gic_dist = NULL;
    struct array msi_frame_list =
        ARRAY_INIT(sizeof(struct acpi_madt_entry_gic_msi_frame *));

    struct range gicv3_redist_discovery_range = RANGE_EMPTY();
    struct array its_list =
        ARRAY_INIT(sizeof(struct acpi_madt_entry_gic_its *));

    uint64_t gicv3_cpu_intr_phys_addr = 0;
#endif /* defined(_-x86_64__) */

    uint32_t length = madt->sdt.length - sizeof(*madt);
    bool found_nmi_lint = false;

    for (uint32_t offset = 0, index = 0;
         offset + sizeof(struct acpi_madt_entry_header) <= length;
         offset += iter->length, index++)
    {
        iter =
            (const struct acpi_madt_entry_header *)&madt->madt_entries[offset];

        switch (iter->kind) {
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_APIC: {
                if (iter->length != sizeof(struct acpi_madt_entry_cpu_lapic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local-apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_lapic *const hdr =
                    (const struct acpi_madt_entry_cpu_lapic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found madt-entry cpu local-apic\n"
                       "\tapic id: %" PRIu8 "\n"
                       "\tprocessor id: %" PRIu8 "\n",
                       hdr->apic_id,
                       hdr->processor_id);

                const struct lapic_info lapic_info = {
                    .apic_id = hdr->apic_id,
                    .processor_id = hdr->processor_id,
                    .enabled = hdr->flags & __ACPI_MADT_ENTRY_CPU_LAPIC_ENABLED,
                    .online_capable =
                        hdr->flags & __ACPI_MADT_ENTRY_CPU_LAPIC_ONLINE_CAPABLE
                };

                lapic_add(&lapic_info);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found local-apic entry. ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_IO_APIC: {
                if (iter->length != sizeof(struct acpi_madt_entry_ioapic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid io-apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_ioapic *const hdr =
                    (const struct acpi_madt_entry_ioapic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry io-apic\n"
                       "\tapic id: %" PRIu8 "\n"
                       "\tbase: 0x%" PRIx32 "\n"
                       "\tglobal system interrupt base: 0x%" PRIx32 "\n",
                       hdr->apic_id,
                       hdr->base,
                       hdr->gsib);

                ioapic_add(hdr->apic_id, hdr->base, hdr->gsib);
            #else
                printk(LOGLEVEL_WARN, "madt: found ioapic entry. ignoring\n");
            #endif /* defined(__x86_64__) */
                continue;
            }
            case ACPI_MADT_ENTRY_KIND_INT_SRC_OVERRIDE: {
                if (iter->length != sizeof(struct acpi_madt_entry_iso)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid int-src override entry at "
                           "index: %" PRIu32,
                           index);
                    continue;
                }

                const struct acpi_madt_entry_iso *const hdr =
                    (const struct acpi_madt_entry_iso *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry interrupt source override\n"
                       "\tbus source: %" PRIu8 "\n"
                       "\tirq source: %" PRIu8 "\n"
                       "\tglobal system interrupt: %" PRIu8 "\n"
                       "\tflags: 0x%" PRIx16 "\n"
                       "\t\tactive-low: %s\n"
                       "\t\tlevel-triggered: %s\n",
                       hdr->bus_source,
                       hdr->irq_source,
                       hdr->gsi,
                       hdr->flags,
                       (hdr->flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW) != 0 ?
                        "yes" : "no",
                       (hdr->flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER) != 0 ?
                        "yes" : "no");

                const struct apic_iso_info info = {
                    .bus_src = hdr->bus_source,
                    .irq_src = hdr->irq_source,
                    .gsi = hdr->gsi,
                    .flags = hdr->flags
                };

                assert_msg(array_append(&get_acpi_info_mut()->iso_list, &info),
                           "madt: failed to add apic iso-info to array");
                continue;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT_SRC: {
                if (iter->length != sizeof(struct acpi_madt_entry_nmi_src)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid nmi source entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_nmi_src *const hdr =
                    (const struct acpi_madt_entry_nmi_src *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry non-maskable interrupt source\n"
                       "\tsource: %" PRIu8 "\n"
                       "\tglobal system interrupt: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx16 "\n"
                       "\t\tactive-low: %s\n"
                       "\t\tlevel-triggered: %s\n",
                       hdr->source,
                       hdr->gsi,
                       hdr->flags,
                       hdr->flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW ?
                        "yes" : "no",
                       hdr->flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER ?
                        "yes" : "no");
                continue;
            }
            case ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INT: {
                if (iter->length != sizeof(struct acpi_madt_entry_nmi)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid nmi override entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_entry_nmi *const hdr =
                    (const struct acpi_madt_entry_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry non-maskable interrupt\n"
                       "\tprocessor: %" PRIu8 "\n"
                       "\tflags: %" PRIu16 "\n"
                       "\tlint: %" PRIu8 "\n",
                       hdr->processor,
                       hdr->flags,
                       hdr->lint);

                if (found_nmi_lint) {
                    if (get_acpi_info()->nmi_lint != hdr->lint) {
                        printk(LOGLEVEL_INFO,
                               "madt: found multiple differing nmi-lint "
                               "values (%" PRIu8 " vs %" PRIu8 "\n",
                               get_acpi_info()->nmi_lint,
                               hdr->lint);
                    }

                    break;
                }

                get_acpi_info_mut()->nmi_lint = hdr->lint;
                found_nmi_lint = true;

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_LOCAL_APIC_ADDR_OVERRIDE: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_lapic_addr_override))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid lapic addr override entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_lapic_addr_override *const hdr =
                    (const struct acpi_madt_entry_lapic_addr_override *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-apic address override\n"
                       "\tbase: 0x%" PRIx64 "\n",
                       hdr->base);

                local_apic_base = hdr->base;
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found local-apic addr override entry. "
                       "ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_cpu_local_x2apic))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local x2apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_local_x2apic *const hdr =
                    (const struct acpi_madt_entry_cpu_local_x2apic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-x2apic\n"
                       "\tacpi id: %" PRIu32 "\n"
                       "\tx2acpi id: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n",
                       hdr->acpi_uid,
                       hdr->x2apic_id,
                       hdr->flags);
            #else
                printk(LOGLEVEL_WARN, "madt: found x2apic entry. ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC_NMI: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_cpu_local_x2apic_nmi))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local x2apic nmi entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct acpi_madt_entry_cpu_local_x2apic_nmi *const hdr =
                    (const struct acpi_madt_entry_cpu_local_x2apic_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-x2apic nmi\n"
                       "\tacpi uid: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\tx2acpi lint: %" PRIu32 "\n",
                       hdr->acpi_uid,
                       hdr->flags,
                       hdr->local_x2apic_lint);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found x2apic nmi entry. ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_CPU_INTERFACE: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_gic_cpu_interface))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic cpu-interface entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_cpu_interface *const cpu =
                    (const struct acpi_madt_entry_gic_cpu_interface *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found gic cpu-interface:\n"
                       "\tinterface number: %" PRIu32 "\n"
                       "\tacpi processor id: %" PRIu32 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\t\tcpu enabled: %s\n"
                       "\t\tperf interrupt edge-triggered: %s\n"
                       "\t\tvgic maintenance intr edge-triggered: %s\n"
                       "\tparking protocol version: %" PRIu32 "\n"
                       "\tperformance interrupt gsiv: %" PRIu32 "\n"
                       "\tparked address: 0x%" PRIx64 "\n"
                       "\tphys base address: 0x%" PRIx64 "\n"
                       "\tgic virt cpu reg address: 0x%" PRIx64 "\n"
                       "\tgic virt ctrl block address: 0x%" PRIx64 "\n"
                       "\tvgic maintenance interrupt: %" PRIu32 "\n"
                       "\tgicr phys base address: 0x%" PRIx64 "\n"
                       "\tmpidr: %" PRIu64 "\n"
                       "\tprocessor power efficiency class: %" PRIu8 "\n"
                       "\tspe overflow interrupt: %" PRIu16 "\n",
                       cpu->cpu_interface_number,
                       cpu->acpi_processor_id,
                       cpu->flags,
                       cpu->flags & __ACPI_MADT_ENTRY_GIC_CPU_ENABLED ?
                        "yes" : "no",
                       cpu->flags &
                        __ACPI_MADT_ENTRY_GIC_CPU_PERF_INTR_EDGE_TRIGGER ?
                            "yes" : "no",
                       cpu->flags &
                        __ACPI_MADT_ENTRY_GIC_CPU_VGIC_INTR_EDGE_TRIGGER ?
                            "yes" : "no",
                       cpu->parking_protocol_version,
                       cpu->perf_interrupt_gsiv,
                       cpu->parked_address,
                       cpu->phys_base_address,
                       cpu->gic_virt_cpu_reg_address,
                       cpu->gic_virt_ctrl_block_reg_address,
                       cpu->vgic_maintenance_interrupt,
                       cpu->gicr_phys_base_address,
                       cpu->mpidr,
                       cpu->processor_power_efficiency_class,
                       cpu->spe_overflow_interrupt);

                if (cpu->phys_base_address == 0) {
                    printk(LOGLEVEL_WARN,
                           "madt: gic cpu-interface phys-address is zero\n");
                    continue;
                }

                if (gicv3_cpu_intr_phys_addr != 0) {
                    if (gicv3_cpu_intr_phys_addr != cpu->phys_base_address) {
                        printk(LOGLEVEL_WARN,
                               "madt: gic cpu-interface has multiple "
                               "conflicitng phys-addresses: 0x%" PRIx64 " vs "
                               "0x%" PRIx64 "\n",
                               gicv3_cpu_intr_phys_addr,
                               cpu->phys_base_address);
                        return;
                    }
                } else {
                    gicv3_cpu_intr_phys_addr = cpu->phys_base_address;
                }
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic cpu-interface entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_DISTRIBUTOR: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_gic_distributor))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic distributor entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_distributor *const dist =
                    (const struct acpi_madt_entry_gic_distributor *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found gic distributor\n"
                       "\tgic hardware id: %" PRIu32 "\n"
                       "\tphys base address: 0x%" PRIx64 "\n"
                       "\tsystem vector base: %" PRIu32 "\n"
                       "\tgic version: %" PRIu8 "\n",
                       dist->gic_hardware_id,
                       dist->phys_base_address,
                       dist->sys_vector_base,
                       dist->gic_version);

                gic_dist = dist;
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic distributor entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_MSI_FRAME: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_gic_msi_frame))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic msi-frame entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_msi_frame *const frame =
                    (const struct acpi_madt_entry_gic_msi_frame *)iter;

                if (!array_append(&msi_frame_list, &frame)) {
                    printk(LOGLEVEL_INFO,
                           "madt: failed to add msi-frame to list\n");
                    continue;
                }

                printk(LOGLEVEL_INFO,
                       "madt: found msi-frame\n"
                       "\tmsi frame id: %" PRIu32 "\n"
                       "\tphys base address: 0x%" PRIx64 "\n"
                       "\tflags: 0x%" PRIx8 "\n"
                       "\t\toverride msi_typer: %s\n"
                       "\tspi count: %" PRIu16 "\n"
                       "\tspi base: %" PRIu16 "\n",
                       frame->msi_frame_id,
                       frame->phys_base_address,
                       frame->flags,
                       frame->flags &
                        __ACPI_MADT_GICMSI_FRAME_OVERR_MSI_TYPERR ?
                            "yes" : "no",
                       frame->spi_count,
                       frame->spi_base);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic msi-frame entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_REDISTRIBUTOR: {
                if (iter->length
                        != sizeof(struct acpi_madt_entry_gicv3_redistributor))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gicv3 redistributor entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gicv3_redistributor *const redist =
                    (const struct acpi_madt_entry_gicv3_redistributor *)iter;

                gicv3_redist_discovery_range =
                    RANGE_INIT(redist->discovery_range_base_address,
                               redist->dicovery_range_length);

                printk(LOGLEVEL_INFO,
                       "madt: found gicv3 redistributor:\n"
                       "\tdiscovery range: " RANGE_FMT "\n",
                       RANGE_FMT_ARGS(gicv3_redist_discovery_range));

                continue;
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gicv3 redistributor entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_GIC_INTR_TRANSLATE_SERVICE: {
                if (iter->length != sizeof(struct acpi_madt_entry_gic_its)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic interrupt translation service "
                           "entry at index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const struct acpi_madt_entry_gic_its *const its =
                    (const struct acpi_madt_entry_gic_its *)iter;

                if (!array_append(&its_list, &its)) {
                    printk(LOGLEVEL_INFO,
                           "madt: failed to add gic its entry to list\n");
                    continue;
                }

                printk(LOGLEVEL_INFO,
                       "madt: found gic interrupt translation service:\n"
                       "\tid: %" PRIu32 "\n"
                       "\tphysical base address: %p\n",
                       its->id,
                       (void *)its->phys_base_address);

                continue;
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gicv3 its entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case ACPI_MADT_ENTRY_KIND_MULTIPROCESSOR_WAKEUP_SERVICE:
                continue;
        #if defined(__riscv64)
            case ACPI_MADT_ENTRY_KIND_RISCV_HART_IRQ_CNTRLR: {
                if (iter->length
                        != sizeof(struct acpi_madt_riscv_hart_irq_controller))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid riscv-hart irq-controllers at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const struct acpi_madt_riscv_hart_irq_controller *const cntrl =
                    (const struct acpi_madt_riscv_hart_irq_controller *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found riscv hart irq controller\n"
                       "\tversion: %" PRIu8 "\n"
                       "\tflags: 0x%" PRIx32 "\n"
                       "\t\tenabled: %s\n"
                       "\t\tonline capable: %s\n"
                       "\thart id: %" PRIu64 "\n"
                       "\tacpi processor uid: %" PRIu32 "\n",
                       cntrl->version,
                       cntrl->flags,
                       cntrl->flags &
                            __ACPI_MADT_RISCV_HART_IRQ_CNTRLR_ENABLED ?
                                "yes" : "no",
                        cntrl->flags &
                            __ACPI_MADT_RISCV_HART_IRQ_ONLINE_CAPABLE ?
                                "yes" : "no",
                       cntrl->hart_id,
                       cntrl->acpi_proc_uid);

                continue;
            }
        #endif /* defined(__riscv64) */
        }

        printk(LOGLEVEL_INFO,
               "madt: found invalid entry: %" PRIu32 "\n",
               iter->kind);
    }

#if defined(__x86_64__)
    assert_msg(local_apic_base != 0,
               "madt: failed to find local-apic registers");

    apic_init(local_apic_base);
#elif defined(__aarch64__)
    assert_msg(gic_dist != NULL, "madt: failed to find gic-distributor");
    gic_set_version(gic_dist->gic_version);

    switch (gic_dist->gic_version) {
        case 2:
            gicv2_init_from_acpi(gic_dist->phys_base_address);
            gicv2_init_on_this_cpu(gicv3_cpu_intr_phys_addr, PAGE_SIZE);

            array_foreach(&its_list,
                          struct acpi_madt_entry_gic_msi_frame *,
                          frame)
            {
                gicv2_add_msi_frame((*frame)->phys_base_address);
            }

            array_destroy(&its_list);
            array_destroy(&msi_frame_list);

            return;
        case 3:
            assert_msg(!range_empty(gicv3_redist_discovery_range),
                       "madt; couldn't find gicv3 redistributor memory region");

            gicv3_init_from_info(gic_dist->phys_base_address,
                                 gicv3_redist_discovery_range);

            array_foreach(&its_list, struct acpi_madt_entry_gic_its *, its) {
                gic_its_init_from_info((*its)->id, (*its)->phys_base_address);
            }

            array_destroy(&its_list);
            array_destroy(&msi_frame_list);

            return;
    }

    verify_not_reached();
#endif /* defined(__x86_64__) */
}