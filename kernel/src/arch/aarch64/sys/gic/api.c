/*
 * kernel/src/arch/aarch64/sys/gic/api.c
 * © suhas pai
 */

#include "dev/dtb/gic_compat.h"
#include "dev/dtb/init.h"

#include "sys/gic/v2.h"
#include "sys/gic/v3.h"

#include "sys/boot.h"

static uint32_t g_version = 0;

__optimize(3) void gicd_mask_irq(const irq_number_t irq) {
    switch (g_version) {
        case 2:
            gicdv2_mask_irq(irq);
            return;
        case 3:
            gicdv3_mask_irq(irq);
            return;
    }

    verify_not_reached();
}

__optimize(3) void gicd_unmask_irq(const irq_number_t irq) {
    switch (g_version) {
        case 2:
            gicdv2_unmask_irq(irq);
            return;
        case 3:
            gicdv3_unmask_irq(irq);
            return;
    }

    verify_not_reached();
}

__optimize(3) isr_vector_t
gicd_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    switch (g_version) {
        case 2:
            return gicdv2_alloc_msi_vector();
        case 3:
            return gicdv3_alloc_msi_vector(device, msi_index);
    }

    verify_not_reached();
}

__optimize(3)
void gicd_free_msi_vector(const isr_vector_t vector, const uint16_t msi_index) {
    switch (g_version) {
        case 2:
            gicdv2_free_msi_vector(vector);
            return;
        case 3:
            gicdv3_free_msi_vector(vector, msi_index);
            return;
    }

    verify_not_reached();
}

__optimize(3)
void gicd_set_irq_affinity(const irq_number_t irq, const uint8_t affinity) {
    switch (g_version) {
        case 2:
            gicdv2_set_irq_affinity(irq, affinity);
            return;
        case 3:
            gicdv3_set_irq_affinity(irq, affinity);
            return;
    }

    verify_not_reached();
}

__optimize(3) void
gicd_set_irq_trigger_mode(const irq_number_t irq,
                          const enum irq_trigger_mpde mode)
{
    switch (g_version) {
        case 2:
            gicdv2_set_irq_trigger_mode(irq, mode);
            return;
        case 3:
            gicdv3_set_irq_trigger_mode(irq, mode);
            return;
    }

    verify_not_reached();
}

__optimize(3)
void gicd_set_irq_priority(const irq_number_t irq, const uint8_t priority) {
    switch (g_version) {
        case 2:
            gicdv2_set_irq_priority(irq, priority);
            return;
        case 3:
            gicdv3_set_irq_priority(irq, priority);
            return;
    }

    verify_not_reached();
}

__optimize(3)
void gicd_send_ipi(const struct cpu_info *const cpu, const uint8_t int_no) {
    switch (g_version) {
        case 2:
            gicdv2_send_ipi(cpu, int_no);
            return;
        case 3:
            gicdv3_send_ipi(cpu, int_no);
            return;
    }

    verify_not_reached();
}

__optimize(3) void gicd_send_sipi(const uint8_t int_no) {
    switch (g_version) {
        case 2:
            gicdv2_send_sipi(int_no);
            return;
        case 3:
            gicdv3_send_sipi(int_no);
            return;
    }

    verify_not_reached();
}

__optimize(3)
volatile uint64_t *gicd_get_msi_address(const isr_vector_t vector) {
    switch (g_version) {
        case 2:
            return gicdv2_get_msi_address(vector);
        case 3:
            return gicdv3_get_msi_address(vector);
    }

    verify_not_reached();
}

__optimize(3) enum isr_msi_support gicd_get_msi_support() {
    switch (g_version) {
        case 2:
            return gicdv2_get_msi_support();
        case 3:
            return gicdv3_get_msi_support();
    }

    verify_not_reached();
}

__optimize(3) irq_number_t gic_cpu_get_irq_number(uint8_t *const cpu_id_out) {
    switch (g_version) {
        case 2:
            return gicv2_cpu_get_irq_number(cpu_id_out);
        case 3:
            return gicv3_cpu_get_irq_number(cpu_id_out);
    }

    verify_not_reached();
}

__optimize(3) uint32_t gic_cpu_get_irq_priority() {
    switch (g_version) {
        case 2:
            return gicv2_cpu_get_irq_priority();
        case 3:
            return gicv3_cpu_get_irq_priority();
    }

    verify_not_reached();
}

__optimize(3)
void gic_cpu_eoi(const uint8_t cpu_id, const irq_number_t irq_number) {
    switch (g_version) {
        case 2:
            gicv2_cpu_eoi(cpu_id, irq_number);
            return;
        case 3:
            gicv3_cpu_eoi(cpu_id, irq_number);
            return;
    }

    verify_not_reached();
}

void gic_init_on_this_cpu(const uint64_t phys_addr, const uint64_t size) {
    switch (g_version) {
        case 2:
            gicv2_init_on_this_cpu(phys_addr, size);
            return;
        case 3:
            gicv3_init_on_this_cpu();
            return;
    }

    verify_not_reached();
}

__optimize(3) void gic_set_version(const uint8_t version) {
    g_version = version;
}

void gic_init_from_dtb() {
    struct devicetree *const tree = dtb_get_tree();
    if (boot_get_dtb() == NULL) {
        return;
    }

    struct dtb_driver gicv3_driver = {
        .init = gicv3_init_from_dtb,
        .match_flags = __DTB_DRIVER_MATCH_COMPAT,

        .compat_list = gicv3_compat_sv_list,
        .compat_count = countof(gicv3_compat_sv_list),
    };

    if (dtb_init_nodes_for_driver(&gicv3_driver, tree, tree->root)) {
        return;
    }

    struct dtb_driver gic_driver = {
        .init = gicv2_init_from_dtb,
        .match_flags = __DTB_DRIVER_MATCH_COMPAT,

        .compat_list = gicv2_compat_sv_list,
        .compat_count = countof(gicv2_compat_sv_list),
    };

    assert_msg(dtb_init_nodes_for_driver(&gic_driver, tree, tree->root),
               "dtb: gicv2/gicv3 not found or was malformed");
}
