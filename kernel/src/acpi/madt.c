/*
 * kernel/src/acpi/madt.c
 * © suhas pai
 */

#include "acpi/api.h"

#if defined(__x86_64__)
    #include "apic/ioapic.h"
    #include "apic/lapic.h"
    #include "apic/init.h"

    #include "sys/isr.h"
#endif /* defined(__x86_64__) */

#include "dev/printk.h"

#if defined(__aarch64__)
    #include "sys/gic/api.h"
    #include "sys/gic/its.h"

    #include "sys/gic/v2.h"
    #include "sys/gic/v3.h"

    #include "mm/mm_types.h"
#elif defined(__riscv64)
    #include "cpu/info.h"
    #include "mm/mmio.h"

    #include "sys/aplic.h"
    #include "sys/imsic.h"
#endif /* defined(__aarch64__) */

void madt_init(const struct os_acpi_madt *const madt) {
    const struct os_acpi_madt_entry_header *iter = nullptr;

#if defined(__x86_64__)
    uint64_t local_apic_base = madt->local_apic_base;
#elif defined(__aarch64__)
    const struct os_acpi_madt_entry_gic_distributor *gic_dist = nullptr;
    struct array msi_frame_list =
        ARRAY_INIT(sizeof(struct acpi_madt_entry_gic_msi_frame *));

    struct range gicv3_redist_discovery_range = RANGE_EMPTY();
    struct array its_list =
        ARRAY_INIT(sizeof(struct acpi_madt_entry_gic_its *));

    uint64_t gicv2_cpu_intr_phys_addr = 0;
#elif defined(__riscv64)
    struct array plic_list =
        ARRAY_INIT(sizeof(struct os_acpi_madt_riscv_plic *));
    struct array aplic_list =
        ARRAY_INIT(sizeof(struct os_acpi_madt_riscv_aplic *));
    struct array hart_irq_ctlr_list =
        ARRAY_INIT(sizeof(struct os_acpi_madt_riscv_hart_irq_controller *));

    const struct os_acpi_madt_riscv_imsic *supervisor_imsic = nullptr;
#endif /* defined(_-x86_64__) */

    uint32_t length = madt->sdt.length - sizeof(*madt);
    bool found_nmi_lint = false;

    for (uint32_t offset = 0, index = 0;
         offset + sizeof(struct os_acpi_madt_entry_header) <= length;
         offset += iter->length, index++)
    {
        iter =
            (const struct os_acpi_madt_entry_header *)
                &madt->madt_entries[offset];

        switch (iter->kind) {
            case OS_ACPI_MADT_ENTRY_KIND_CPU_LOCAL_APIC: {
                if (iter->length !=
                        sizeof(struct os_acpi_madt_entry_cpu_lapic))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local-apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct os_acpi_madt_entry_cpu_lapic *const hdr =
                    (const struct os_acpi_madt_entry_cpu_lapic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found madt-entry cpu local-apic\n"
                       "\t" "apic id: %" PRIu8 "\n"
                       "\t" "processor id: %" PRIu8 "\n",
                       hdr->apic_id,
                       hdr->processor_id);

                const struct lapic_info lapic_info = {
                    .apic_id = hdr->apic_id,
                    .processor_id = hdr->processor_id,
                    .enabled =
                        hdr->flags & __OS_ACPI_MADT_ENTRY_CPU_LAPIC_ENABLED,
                    .online_capable =
                        hdr->flags &
                            __OS_ACPI_MADT_ENTRY_CPU_LAPIC_ONLINE_CAPABLE
                };

                lapic_add(&lapic_info);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found local-apic entry. ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_IO_APIC: {
                if (iter->length != sizeof(struct os_acpi_madt_entry_ioapic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid io-apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct os_acpi_madt_entry_ioapic *const hdr =
                    (const struct os_acpi_madt_entry_ioapic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry io-apic\n"
                       "\t" "apic id: %" PRIu8 "\n"
                       "\t" "base: 0x%" PRIx32 "\n"
                       "\t" "global system interrupt base: 0x%" PRIx32 "\n",
                       hdr->apic_id,
                       hdr->base,
                       hdr->gsib);

                ioapic_add(hdr->apic_id, hdr->base, hdr->gsib);
            #else
                printk(LOGLEVEL_WARN, "madt: found ioapic entry. ignoring\n");
            #endif /* defined(__x86_64__) */
                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_INTR_SRC_OVERRIDE: {
                if (iter->length != sizeof(struct os_acpi_madt_entry_iso)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid int-src override entry at "
                           "index: %" PRIu32,
                           index);
                    continue;
                }

                const __auto_type hdr =
                    (const struct os_acpi_madt_entry_iso *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry interrupt source override\n"
                       "\t" "bus source: %" PRIu8 "\n"
                       "\t" "irq source: %" PRIu8 "\n"
                       "\t" "global system interrupt: %" PRIu8 "\n"
                       "\t" "flags: 0x%" PRIx16 "\n"
                       "\t\t" "active-low: %s\n"
                       "\t\t" "level-triggered: %s\n",
                       hdr->bus_source,
                       hdr->irq_source,
                       hdr->gsi,
                       hdr->flags,
                       (hdr->flags &
                        __OS_ACPI_MADT_ENTRY_ISO_ACTIVE_LOW) != 0 ?
                            "yes" : "no",
                       (hdr->flags &
                        __OS_ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER) != 0 ?
                            "yes" : "no");

                const struct apic_iso_info info = {
                    .bus_src = hdr->bus_source,
                    .irq_src = hdr->irq_source,
                    .gsi = hdr->gsi,
                    .flags = hdr->flags
                };

                assert_msg(array_add(&get_acpi_info_mut()->iso_list, &info),
                           "madt: failed to add apic iso-info to array");
                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INTR_SRC: {
                if (iter->length != sizeof(struct os_acpi_madt_entry_nmi_src)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid nmi source entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const __auto_type hdr =
                    (const struct os_acpi_madt_entry_nmi_src *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry non-maskable interrupt source\n"
                       "\t" "source: %" PRIu8 "\n"
                       "\t" "global system interrupt: %" PRIu32 "\n"
                       "\t" "flags: 0x%" PRIx16 "\n"
                       "\t\t" "active-low: %s\n"
                       "\t\t" "level-triggered: %s\n",
                       hdr->source,
                       hdr->gsi,
                       hdr->flags,
                       hdr->flags & __OS_ACPI_MADT_ENTRY_ISO_ACTIVE_LOW ?
                        "yes" : "no",
                       hdr->flags & __OS_ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER ?
                        "yes" : "no");
                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_NON_MASKABLE_INTR: {
                if (iter->length != sizeof(struct os_acpi_madt_entry_nmi)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid nmi override entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

                const __auto_type hdr =
                    (const struct os_acpi_madt_entry_nmi *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry non-maskable interrupt\n"
                       "\t" "processor: %" PRIu8 "\n"
                       "\t" "flags: %" PRIu16 "\n"
                       "\t" "lint: %" PRIu8 "\n",
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

                    continue;
                }

                get_acpi_info_mut()->nmi_lint = hdr->lint;
                found_nmi_lint = true;

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_LOCAL_APIC_ADDR_OVERRIDE: {
                const __auto_type size =
                    sizeof(struct os_acpi_madt_entry_lapic_addr_override);

                if (iter->length != size) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid lapic addr override entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const __auto_type hdr =
                    (const struct os_acpi_madt_entry_lapic_addr_override *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-apic address override\n"
                       "\t" "base: 0x%" PRIx64 "\n",
                       hdr->base);

                local_apic_base = hdr->base;
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found local-apic addr override entry. "
                       "ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC: {
                if (iter->length
                        != sizeof(struct os_acpi_madt_entry_cpu_local_x2apic))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local x2apic entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const struct os_acpi_madt_entry_cpu_local_x2apic *const hdr =
                    (const struct os_acpi_madt_entry_cpu_local_x2apic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-x2apic\n"
                       "\t" "acpi id: %" PRIu32 "\n"
                       "\t" "x2acpi id: %" PRIu32 "\n"
                       "\t" "flags: 0x%" PRIx32 "\n",
                       hdr->acpi_uid,
                       hdr->x2apic_id,
                       hdr->flags);
            #else
                printk(LOGLEVEL_WARN, "madt: found x2apic entry. ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_CPU_LOCAL_X2APIC_NMI: {
                const __auto_type size =
                    sizeof(struct os_acpi_madt_entry_cpu_local_x2apic_nmi);

                if (iter->length != size) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid local x2apic nmi entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__x86_64__)
                const __auto_type hdr =
                    (const struct os_acpi_madt_entry_cpu_local_x2apic_nmi *)
                        iter;

                printk(LOGLEVEL_INFO,
                       "madt: found entry local-x2apic nmi\n"
                       "\t" "acpi uid: %" PRIu32 "\n"
                       "\t" "flags: 0x%" PRIx32 "\n"
                       "\t" "x2acpi lint: %" PRIu32 "\n",
                       hdr->acpi_uid,
                       hdr->flags,
                       hdr->local_x2apic_lint);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found x2apic nmi entry. ignoring\n");
            #endif /* defined(__x86_64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_GIC_CPU_INTERFACE: {
                if (iter->length
                        != sizeof(struct os_acpi_madt_entry_gic_cpu_interface))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic cpu-interface entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const __auto_type cpu =
                    (const struct os_acpi_madt_entry_gic_cpu_interface *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found gic cpu-interface:\n"
                       "\t\t" "interface number: %" PRIu32 "\n"
                       "\t\t" "acpi processor id: %" PRIu32 "\n"
                       "\t\t" "flags: 0x%" PRIx32 "\n"
                       "\t\t\t" "cpu enabled: %s\n"
                       "\t\t\t" "perf interrupt edge-triggered: %s\n"
                       "\t\t\t" "vgic maintenance intr edge-triggered: %s\n"
                       "\t\t" "parking protocol version: %" PRIu32 "\n"
                       "\t\t" "performance interrupt gsiv: %" PRIu32 "\n"
                       "\t\t" "parked address: 0x%" PRIx64 "\n"
                       "\t\t" "phys base address: 0x%" PRIx64 "\n"
                       "\t\t" "gic virt cpu reg address: 0x%" PRIx64 "\n"
                       "\t\t" "gic virt ctrl block address: 0x%" PRIx64 "\n"
                       "\t\t" "vgic maintenance interrupt: %" PRIu32 "\n"
                       "\t\t" "gicr phys base address: 0x%" PRIx64 "\n"
                       "\t\t" "mpidr: %" PRIu64 "\n"
                       "\t\t" "processor power efficiency class: %" PRIu8 "\n"
                       "\t\t" "spe overflow interrupt: %" PRIu16 "\n",
                       cpu->cpu_interface_number,
                       cpu->acpi_processor_id,
                       cpu->flags,
                       cpu->flags & __OS_ACPI_MADT_ENTRY_GIC_CPU_ENABLED ?
                        "yes" : "no",
                       cpu->flags &
                        __OS_ACPI_MADT_ENTRY_GIC_CPU_PERF_INTR_EDGE_TRIGGER ?
                            "yes" : "no",
                       cpu->flags &
                        __OS_ACPI_MADT_ENTRY_GIC_CPU_VGIC_INTR_EDGE_TRIGGER ?
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

                if (gicv2_cpu_intr_phys_addr != 0) {
                    if (gicv2_cpu_intr_phys_addr != cpu->phys_base_address) {
                        printk(LOGLEVEL_WARN,
                               "madt: gic cpu-interface has multiple "
                               "conflicting phys-addresses: 0x%" PRIx64 " vs "
                               "0x%" PRIx64 "\n",
                               gicv2_cpu_intr_phys_addr,
                               cpu->phys_base_address);
                        return;
                    }
                } else {
                    gicv2_cpu_intr_phys_addr = cpu->phys_base_address;
                }
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic cpu-interface entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_GIC_DISTRIBUTOR: {
                if (iter->length
                        != sizeof(struct os_acpi_madt_entry_gic_distributor))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic distributor entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const __auto_type dist =
                    (const struct os_acpi_madt_entry_gic_distributor *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found gic distributor\n"
                       "\t" "gic hardware id: %" PRIu32 "\n"
                       "\t" "phys base address: 0x%" PRIx64 "\n"
                       "\t" "system vector base: %" PRIu32 "\n"
                       "\t" "gic version: %" PRIu8 "\n",
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
            case OS_ACPI_MADT_ENTRY_KIND_GIC_MSI_FRAME: {
                if (iter->length
                        != sizeof(struct os_acpi_madt_entry_gic_msi_frame))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic msi-frame entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const __auto_type frame =
                    (const struct os_acpi_madt_entry_gic_msi_frame *)iter;

                assert (array_add(&msi_frame_list, &frame));
                printk(LOGLEVEL_INFO,
                       "madt: found msi-frame\n"
                       "\t\t" "msi frame id: %" PRIu32 "\n"
                       "\t\t" "phys base address: 0x%" PRIx64 "\n"
                       "\t\t" "flags: 0x%" PRIx8 "\n"
                       "\t\t\t" "override msi-typer: %s\n"
                       "\t\t" "spi count: %" PRIu16 "\n"
                       "\t\t" "spi base: %" PRIu16 "\n",
                       frame->msi_frame_id,
                       frame->phys_base_address,
                       frame->flags,
                       frame->flags &
                        __OS_ACPI_MADT_GICMSI_FRAME_OVERRIDE_MSI_TYPERR ?
                            "yes" : "no",
                       frame->spi_count,
                       frame->spi_base);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gic msi-frame entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_GIC_REDISTRIBUTOR: {
                const __auto_type size =
                    sizeof(struct os_acpi_madt_entry_gicv3_redistributor);

                if (iter->length != size) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gicv3 redistributor entry at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const __auto_type redist =
                    (const struct os_acpi_madt_entry_gicv3_redistributor *)iter;

                if (!range_create_and_verify(
                        redist->discovery_range_base_address,
                        redist->discovery_range_length,
                        &gicv3_redist_discovery_range))
                {
                    printk(LOGLEVEL_INFO,
                           "madt: found gicv3 redistributor: (with overflowing "
                           "range)\n"
                           "\t\t" "discovery range: " RANGE_FMT "\n",
                           RANGE_FMT_ARGS(gicv3_redist_discovery_range));

                    continue;
                }

                printk(LOGLEVEL_INFO,
                       "madt: found gicv3 redistributor:\n"
                       "\t\t" "discovery range: " RANGE_FMT "\n",
                       RANGE_FMT_ARGS(gicv3_redist_discovery_range));
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gicv3 redistributor entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_GIC_INTR_TRANSLATE_SERVICE: {
                if (iter->length != sizeof(struct os_acpi_madt_entry_gic_its)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid gic interrupt translation service "
                           "entry at index: %" PRIu32 "\n",
                           index);
                    continue;
                }
            #if defined(__aarch64__)
                const __auto_type its =
                    (const struct os_acpi_madt_entry_gic_its *)iter;

                assert(array_add(&its_list, &its));
                printk(LOGLEVEL_INFO,
                       "madt: found gic interrupt translation service:\n"
                       "\t\t" "id: %" PRIu32 "\n"
                       "\t\t" "physical base address: %p\n",
                       its->id,
                       (void *)its->phys_base_address);
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found gicv3 its entry. ignoring\n");
            #endif /* defined(__aarch64__) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_MULTIPROCESSOR_WAKEUP_SERVICE:
                continue;
            case OS_ACPI_MADT_ENTRY_KIND_RISCV_HART_IRQ_CONTROLLER: {
                const __auto_type size =
                    sizeof(struct os_acpi_madt_riscv_hart_irq_controller);

                if (iter->length != size) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid riscv-hart irq-controllers at "
                           "index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__riscv64)
                const __auto_type ctrlr =
                    (const struct os_acpi_madt_riscv_hart_irq_controller *)iter;

                struct cpu_info *const cpu = cpu_for_id_mut(ctrlr->hart_id);
                printk(LOGLEVEL_INFO,
                       "madt: found riscv hart irq controller\n"
                       "\t\t" "version: %" PRIu8 "\n"
                       "\t\t" "flags: 0x%" PRIx32 "\n"
                       "\t\t\t" "enabled: %s\n"
                       "\t\t\t" "online capable: %s\n"
                       "\t\t" "hart id: %" PRIu64 "%s\n"
                       "\t\t" "acpi processor uid: %" PRIu32 "\n"
                       "\t\t" "external irq controller id: %" PRIu32 "\n"
                       "\t\t" "imsic base address: %p\n"
                       "\t\t" "imsic size: %" PRIu32 "\n",
                       ctrlr->version,
                       ctrlr->flags,
                       ctrlr->flags &
                            __OS_ACPI_MADT_RISCV_HART_IRQ_CNTRLR_ENABLED ?
                                "yes" : "no",
                        ctrlr->flags &
                            __OS_ACPI_MADT_RISCV_HART_IRQ_ONLINE_CAPABLE ?
                                "yes" : "no",
                       ctrlr->hart_id,
                       cpu != nullptr ? "" : " (cpu not found)",
                       ctrlr->acpi_proc_uid,
                       ctrlr->external_irq_controller_id,
                       (void *)ctrlr->imsic_base,
                       ctrlr->imsic_size);

                if (cpu != nullptr) {
                    struct mmio_region *const imsic_mmio =
                        vmap_mmio(RANGE_INIT(ctrlr->imsic_base,
                                             ctrlr->imsic_size),
                                  PROT_READ | PROT_WRITE,
                                  /*flags=*/0);

                    assert_msg(imsic_mmio != nullptr,
                               "madt: failed to map imsic page\n");

                    cpu->imsic_phys = ctrlr->imsic_base;
                    cpu->imsic_page = imsic_mmio->base;
                }

                assert(array_add(&hart_irq_ctlr_list, &ctrlr));
            #else
                printk(LOGLEVEL_WARN,
                       "madt: found riscv hart irq controller entry. "
                       "ignoring\n");
            #endif /* defined(__riscv64) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_RISCV_IMSIC: {
                if (iter->length != sizeof(struct os_acpi_madt_riscv_imsic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid riscv imsic at index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__riscv64)
                const __auto_type imsic =
                    (const struct os_acpi_madt_riscv_imsic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found riscv imsic\n"
                       "\t\t" "version: %" PRIu8 "\n"
                       "\t\t" "flags: 0x%" PRIx32 "\n"
                       "\t\t" "guest node irq identity count: %" PRIu16 "\n"
                       "\t\t" "supervisor node irq identity "
                                "count: %" PRIu16 "\n"
                       "\t\t" "guest index bits: %" PRIu8 "\n"
                       "\t\t" "hart index bits: %" PRIu8 "\n"
                       "\t\t" "group index bits: %" PRIu8 "\n"
                       "\t\t" "group index shift: %" PRIu8 "\n",
                       imsic->version,
                       imsic->flags,
                       imsic->guest_node_irq_identity_count,
                       imsic->supervisor_node_irq_identity_count,
                       imsic->guest_index_bits,
                       imsic->hart_index_bits,
                       imsic->group_index_bits,
                       imsic->group_index_shift);

                supervisor_imsic = imsic;
            #else
                printk(LOGLEVEL_WARN, "madt: found riscv imsic. ignoring ");
            #endif /* defined(__riscv64) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_RISCV_APLIC: {
                if (iter->length != sizeof(struct os_acpi_madt_riscv_aplic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid riscv aplic at index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__riscv64)
                const __auto_type aplic =
                    (const struct os_acpi_madt_riscv_aplic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found riscv aplic\n"
                       "\t\t" "version: %" PRIu8 "\n"
                       "\t\t" "id: %" PRIu8 "\n"
                       "\t\t" "flags: 0x%" PRIx32 "\n"
                       "\t\t" "hardware id: %" PRIu64 "\n"
                       "\t\t" "idc count: %" PRIu16 "\n"
                       "\t\t" "external irq source count: %" PRIu16 "\n"
                       "\t\t" "gsi base: %" PRIu32 "\n"
                       "\t\t" "aplic base: %p\n"
                       "\t\t" "aplic size: %" PRIu32 "\n",
                       aplic->version,
                       aplic->id,
                       aplic->flags,
                       aplic->hardware_id,
                       aplic->idc_count,
                       aplic->ext_irq_source_count,
                       aplic->gsi_base,
                       (void *)aplic->aplic_base,
                       aplic->aplic_size);

                assert(array_add(&aplic_list, &aplic));
            #else
                printk(LOGLEVEL_WARN, "madt: found riscv aplic. ignoring ");
            #endif /* defined(__riscv64) */

                continue;
            }
            case OS_ACPI_MADT_ENTRY_KIND_RISCV_PLIC: {
                if (iter->length != sizeof(struct os_acpi_madt_riscv_plic)) {
                    printk(LOGLEVEL_INFO,
                           "madt: invalid riscv plic at index: %" PRIu32 "\n",
                           index);
                    continue;
                }

            #if defined(__riscv64)
                const __auto_type plic =
                    (const struct os_acpi_madt_riscv_plic *)iter;

                printk(LOGLEVEL_INFO,
                       "madt: found riscv plic\n"
                       "\t\t" "version: %" PRIu8 "\n"
                       "\t\t" "id: %" PRIu8 "\n"
                       "\t\t" "hardware id: %" PRIu64 "\n"
                       "\t\t" "total external irq sources "
                                "supported: %" PRIu16 "\n"
                       "\t\t" "max priority: %" PRIu8 "\n"
                       "\t\t" "flags: %" PRIu8 "\n"
                       "\t\t" "plic base: %p\n"
                       "\t\t" "plic size: %" PRIu32 "\n"
                       "\t\t" "gsi base: %" PRIu32 "\n",
                       plic->version,
                       plic->id,
                       plic->hardware_id,
                       plic->total_ext_irq_source_supported,
                       plic->max_prio,
                       plic->flags,
                       (void *)plic->plic_base,
                       plic->plic_size,
                       plic->gsi_base);

                assert(array_add(&plic_list, &plic));
            #else
                printk(LOGLEVEL_WARN, "madt: found riscv plic. ignoring ");
            #endif /* defined(__riscv64) */

                continue;
            }
        }

        printk(LOGLEVEL_INFO,
               "madt: found invalid entry: %" PRIu32 "\n",
               iter->kind);
    }

    #if defined(__x86_64__)
        assert_msg(local_apic_base != 0,
                   "madt: failed to find local-apic registers");

        apic_init(local_apic_base);
        isr_setup_irq_pins();
    #elif defined(__aarch64__)
        assert_msg(gic_dist != nullptr, "madt: failed to find gic-distributor");
        gic_set_version(gic_dist->gic_version);

        switch (gic_dist->gic_version) {
            case 2: {
                struct range gicv2_cpu_intr_range = RANGE_EMPTY();
                if (!range_create_and_verify(gicv2_cpu_intr_phys_addr,
                                             PAGE_SIZE,
                                             &gicv2_cpu_intr_range))
                {
                    printk(LOGLEVEL_WARN,
                           "madt: specified gicv2 cpu-interface range "
                           "overflows\n");

                    array_destroy(&its_list);
                    array_destroy(&msi_frame_list);

                    return;
                }

                gicv2_init_from_info(gicv2_cpu_intr_range,
                                     gic_dist->phys_base_address);

                array_foreach(&its_list,
                              struct os_acpi_madt_entry_gic_msi_frame *,
                              frame)
                {
                    gicv2_add_msi_frame((*frame)->phys_base_address);
                }

                array_destroy(&its_list);
                array_destroy(&msi_frame_list);

                return;
            }
            case 3:
                assert_msg(!range_empty(gicv3_redist_discovery_range),
                           "madt; couldn't find gicv3 redistributor memory "
                           "region");

                gicv3_init_from_info(gic_dist->phys_base_address,
                                     gicv3_redist_discovery_range);

                array_foreach(&its_list,
                              struct os_acpi_madt_entry_gic_its *,
                              its)
                {
                    gic_its_init_from_info((*its)->id,
                                           (*its)->phys_base_address);
                }

                array_destroy(&its_list);
                array_destroy(&msi_frame_list);

                return;
        }

        panic("GIC with unsupported version %" PRIu8 " found",
              gic_dist->gic_version);
    #elif defined(__riscv64)
        if (!array_empty(aplic_list)) {
            assert_msg(supervisor_imsic != nullptr,
                    "madt: no imsic found, required for aplic\n");

            array_foreach(&hart_irq_ctlr_list,
                          struct os_acpi_madt_riscv_hart_irq_controller *,
                          ctrlr_iter)
            {
                struct range range = RANGE_EMPTY();
                const __auto_type ctrlr= *ctrlr_iter;

                if (!range_create_and_verify(ctrlr->imsic_base,
                                             ctrlr->imsic_size,
                                             &range))
                {
                    printk(LOGLEVEL_WARN,
                           "madt: found hart irq controller with overflowing "
                           "imsic range\n");
                    continue;
                }

                struct cpu_info *const cpu = cpu_for_id_mut(ctrlr->hart_id);
                if (cpu == nullptr) {
                    printk(LOGLEVEL_WARN,
                           "madt: found hart irq controller pointing to "
                           "unknown cpu, with hart-id: %" PRIu64 "\n",
                           ctrlr->hart_id);

                    continue;
                }

                cpu->imsic_phys = ctrlr->imsic_base;
                cpu->imsic_page = imsic_add_region(ctrlr->hart_id, range);
            }

            imsic_init_from_acpi(
                RISCV64_PRIVL_SUPERVISOR,
                supervisor_imsic->guest_index_bits,
                supervisor_imsic->guest_node_irq_identity_count);

            imsic_enable(RISCV64_PRIVL_SUPERVISOR);
            array_foreach(&aplic_list,
                          struct os_acpi_madt_riscv_aplic *,
                          aplic_iter)
            {
                const __auto_type aplic = *aplic_iter;
                struct range range = RANGE_EMPTY();

                if (!range_create_and_verify(aplic->aplic_base,
                                             aplic->aplic_size,
                                             &range))
                {
                    printk(LOGLEVEL_WARN,
                           "madt: found aplic with overflowing register "
                           "range\n");
                    continue;
                }

                aplic_init_from_acpi(range,
                                     aplic->ext_irq_source_count,
                                     aplic->gsi_base);
            }
        } else if (!array_empty(plic_list)) {
            // TODO:
        } else {
            panic("No [a]plics found. Aborting init");
        }

        array_destroy(&aplic_list);
        array_destroy(&plic_list);
        array_destroy(&hart_irq_ctlr_list);
    #endif /* defined(__riscv64) */
}
