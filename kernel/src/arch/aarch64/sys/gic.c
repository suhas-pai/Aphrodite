/*
 * kernel/src/arch/aarch64/sys/gic.c
 * © suhas pai
 */

#include <stdatomic.h>
#include "cpu/info.h"

#include "dev/driver.h"
#include "dev/printk.h"
#include "lib/align.h"
#include "mm/mmio.h"
#include "sys/mmio.h"

#include "gic.h"

#define GIC_DIST_IMPLEMENTER_ID_RESET 0x0001043B

enum gicd_type_shifts {
    GICD_CPU_IMPLD_COUNT_MINUS_ONE_SHIFT = 5,
    GICD_MAX_IMPLD_LOCKABLE_SPIS_SHIFT = 11
};

enum gicd_type_flags {
    __GICD_TYPE_INT_LINES = 0b11111ull,
    __GICD_CPU_IMPLD_COUNT_MINUS_ONE =
        0b111ull << GICD_CPU_IMPLD_COUNT_MINUS_ONE_SHIFT,

    __GICD_IMPLS_SECURITY_EXTENSIONS = 1ull << 10,
    __GICD_MAX_IMPLD_LOCKABLE_SPIS =
        0b11111ull << GICD_MAX_IMPLD_LOCKABLE_SPIS_SHIFT
};

struct gicd_registers {
    volatile uint32_t control;

    volatile const uint32_t interrupt_controller_type;
    volatile const uint32_t dist_implementer_id;

    volatile const char reserved[52];
    volatile uint32_t non_secure_spi_set; // Write-only

    volatile const char reserved_2[4];
    volatile uint32_t non_secure_spi_clear; // Write-only

    volatile const char reserved_3[4];
    volatile uint32_t secure_spi_set; // Write-only

    volatile const char reserved_4[4];
    volatile uint32_t secure_spi_clear; // Write-only

    volatile uint32_t interrupt_group[32];
    volatile const char reserved_5[34];

    volatile uint32_t interrupt_set_enable[32];
    volatile uint32_t interrupt_clear_enable[32];

    volatile uint32_t interrupt_set_pending_enable[32];
    volatile uint32_t interrupt_clear_pending_enable[32];

    volatile uint32_t interrupt_set_active_enable[32];
    volatile uint32_t interrupt_clear_active_enable[32];

    volatile _Atomic uint32_t interrupt_priority[256];
    volatile _Atomic uint32_t interrupt_targets[256];

    // Read-only on SGIs
    volatile uint32_t interrupt_config[64];
    volatile uint32_t interrupt_group_modifiers[64];
    volatile uint32_t non_secure_access_control[64];

    volatile uint32_t software_generated_interrupts[32]; // Write-only
    volatile uint32_t sgi_clear_pending[32];
    volatile uint32_t sgi_set_pending[32];

    volatile const char reserved_11[20608];
    volatile uint64_t interrupt_routing[32];

    volatile const char reserved_12[24062];
    volatile const uint32_t extended_status;

    volatile uint32_t error_test;  // Write-only
    volatile const char reserved_13[124];

    volatile uint32_t shared_peripheral_interrupt_status[32];
    volatile const char reserved_14[16074];

    volatile uint32_t peripheral_id_4;
    volatile uint32_t peripheral_id_5;
    volatile uint32_t peripheral_id_6;
    volatile uint32_t peripheral_id_7;
    volatile uint32_t peripheral_id_0;
    volatile uint32_t peripheral_id_1;
    volatile uint32_t peripheral_id_2;
    volatile uint32_t peripheral_id_3;

    volatile uint32_t component_id_0;
    volatile uint32_t component_id_1;
    volatile uint32_t component_id_2;
    volatile uint32_t component_id_3;
};

enum gic_cpu_interrupt_control_flags {
    __GIC_CPU_INTR_CTRL_ENABLE_GROUP_0 = 1ull << 0,
    __GIC_CPU_INTR_CTRL_ENABLE_GROUP_1 = 1ull << 1,

    __GIC_CPU_INTR_CTRL_ENABLE =
        __GIC_CPU_INTR_CTRL_ENABLE_GROUP_0 |
        __GIC_CPU_INTR_CTRL_ENABLE_GROUP_1,

    // Only valid on GICv2
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_0 = 1ull << 5,
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_1 = 1ull << 6,
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_2 = 1ull << 7,
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_3 = 1ull << 8,

    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE =
        __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_0 |
        __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_1 |
        __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_2 |
        __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_3,

    __GIC_CPU_INTERFACE_CTRL_SPLIT_EOI = 1 << 9,
};

enum gic_cpu_eoi_shifts {
    GIC_CPU_EOI_CPU_ID_SHIFT = 10,
};

enum gic_cpu_eoi_flags {
    __GIC_CPU_EOI_IRQ_ID = 0x3FF,
    __GIC_CPU_EOI_CPU_ID = 0b111 << GIC_CPU_EOI_CPU_ID_SHIFT,
};

struct gic_cpu_interface {
    volatile uint32_t interrupt_control;
    volatile uint32_t priority_mask;
    volatile uint32_t binary_point;
    volatile _Atomic uint32_t interrupt_acknowledge;
    volatile uint32_t end_of_interrupt;
    volatile _Atomic uint32_t running_polarity;

    volatile char reserved[182];
    volatile uint32_t active_priority_base[4];

    volatile char reserved_2[3872];
    volatile uint32_t deactivation;
};

_Static_assert(sizeof(struct gicd_registers) == 0x10000,
               "struct gicd_registers has an incorrect size");

static struct gic_distributor g_dist = {
    .msi_frame_list = ARRAY_INIT(sizeof(struct gic_msi_frame)),
    .version = GICv1,
    .impl_cpu_count = 0,
    .max_impl_lockable_spis = 0
};

#define GICD_CPU_COUNT 8

static struct mmio_region *g_mmio = NULL;
static volatile struct gicd_registers *g_regs = NULL;
static bool g_dist_initialized = false;

static void init_with_regs() {
    mmio_write(&g_regs->control, /*value=*/0);
    if (g_dist.version != GICv1) {
        for (uint16_t i = GIC_SPI_INTERRUPT_START;
             i < g_dist.interrupt_lines_count;
             i += 32)
        {
            const uint32_t index = i / sizeof(uint32_t);
            mmio_write(&g_regs->interrupt_clear_enable[index], 0xFFFFFFFF);
        }
    } else {
        for (uint16_t i = GIC_SPI_INTERRUPT_START;
             i < g_dist.interrupt_lines_count;
             i += 32)
        {
            const uint32_t index = i / sizeof(uint32_t);
            mmio_write(&g_regs->interrupt_clear_active_enable[index],
                       0xFFFFFFFF);
        }
    }

    const uint8_t intr_number = get_cpu_info()->cpu_interface_number;
    const uint32_t mask_four_times =
        (uint32_t)intr_number |
        (uint32_t)intr_number << 8 |
        (uint32_t)intr_number << 16 |
        (uint32_t)intr_number << 24;

    for (uint16_t i = GIC_SPI_INTERRUPT_START;
         i < g_dist.interrupt_lines_count;
         i += 4)
    {
        const uint16_t index = i / sizeof(uint32_t);

        mmio_write(&g_regs->interrupt_priority[index], 0xA0A0A0A0);
        mmio_write(&g_regs->interrupt_targets[index], mask_four_times);
    }

    mmio_write(&g_regs->control, /*value=*/1);
    printk(LOGLEVEL_INFO, "gic: finished initializing gicd\n");
}

void gic_cpu_init(const struct cpu_info *const cpu) {
    if (g_dist.version == GICv1) {
        mmio_write(g_regs->interrupt_clear_active_enable, 0xFFFFFFFF);
    } else {
        mmio_write(&g_regs->interrupt_clear_enable[0], 0xFFFFFFF);
    }

    for (uint16_t i = 0; i != 32; i += 4) {
        const uint16_t index = i / 4;
        mmio_write(&g_regs->interrupt_priority[index], 0xa0a0a0a0);
    }

    if (g_dist.version == GICv1) {
        mmio_write(g_regs->interrupt_set_active_enable, 0xFFFFFFFF);
    } else {
        mmio_write(&g_regs->interrupt_set_enable[0], 0xFFFFFFF);
    }

    volatile struct gic_cpu_interface *const intr = cpu->gic_cpu.interface;

    mmio_write(&intr->priority_mask, 0xF0);
    mmio_write(&intr->active_priority_base[0], /*value=*/0);
    mmio_write(&intr->active_priority_base[1], /*value=*/0);
    mmio_write(&intr->active_priority_base[2], /*value=*/0);
    mmio_write(&intr->active_priority_base[3], /*value=*/0);

    uint32_t interrupt_control = mmio_read(&intr->interrupt_control);
    if (g_dist.version >= GICv2) {
        interrupt_control &= (uint32_t)~__GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE;
    }

    interrupt_control |= __GIC_CPU_INTR_CTRL_ENABLE;
    if (cpu->gic_cpu.mmio->size > PAGE_SIZE) {
        printk(LOGLEVEL_INFO, "gic: using split-eoi for cpu %ld\n", cpu->mpidr);
        interrupt_control |= __GIC_CPU_INTERFACE_CTRL_SPLIT_EOI;
    }

    mmio_write(&intr->interrupt_control, interrupt_control);
    printk(LOGLEVEL_INFO, "gic: initialized cpu interface\n");
}

void gicd_init(const uint64_t phys_base_address, const uint8_t gic_version) {
    if (g_dist_initialized) {
        printk(LOGLEVEL_WARN,
               "gic: attempting to initialize multiple gic distributions\n");
        return;
    }

    struct range mmio_range =
        RANGE_INIT(phys_base_address, sizeof(struct gicd_registers));

    if (!range_align_out(mmio_range, PAGE_SIZE, &mmio_range)) {
        printk(LOGLEVEL_WARN,
               "gic: failed to align gicd register range to page-size\n");
        return;
    }

    g_mmio =
        vmap_mmio(mmio_range,
                  PROT_READ | PROT_WRITE | PROT_DEVICE,
                  /*flags=*/0);

    if (g_mmio == NULL) {
        printk(LOGLEVEL_WARN, "gic: failed to mmio-map dist registers\n");
        return;
    }

    g_regs = g_mmio->base;
    g_dist.version = gic_version;

    assert_msg(g_dist.version >= GICv1 &&
                g_dist.version <= GIC_VERSION_BACK,
               "gic: distributor has an unrecognized version: %d",
               g_dist.version);

    const uint8_t type = mmio_read(&g_regs->interrupt_controller_type);

    g_dist.interrupt_lines_count =
        min(((type & __GICD_TYPE_INT_LINES) + 1) * 32, 1020);
    g_dist.impl_cpu_count =
        ((type & __GICD_CPU_IMPLD_COUNT_MINUS_ONE) >>
            GICD_CPU_IMPLD_COUNT_MINUS_ONE_SHIFT) + 1;
    g_dist.max_impl_lockable_spis =
        (type & __GICD_MAX_IMPLD_LOCKABLE_SPIS) >>
            GICD_MAX_IMPLD_LOCKABLE_SPIS_SHIFT;
    g_dist.supports_security_extensions =
        type & __GICD_IMPLS_SECURITY_EXTENSIONS;

    printk(LOGLEVEL_INFO,
           "gic initialized\n"
           "\tVersion: %" PRIu8 "\n"
           "\tInterrupt Line Count: %" PRIu16 "\n"
           "\tImplemented CPU count: %" PRIu32 "\n"
           "\tMax Implemented Lockable Sets: %" PRIu32 "\n"
           "\tSupports Security Extensions: %s\n",
           g_dist.version,
           g_dist.interrupt_lines_count,
           g_dist.impl_cpu_count,
           g_dist.max_impl_lockable_spis,
           g_dist.supports_security_extensions ? "yes" : "no");

    init_with_regs();
    g_dist_initialized = true;
}

struct gic_msi_frame *gicd_add_msi(const uint64_t phys_base_address) {
    if (!has_align(phys_base_address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
                "madt: gic msi frame's physical base address (%p) is not "
                "aligned to the page-size (%" PRIu32 ")\n",
                (void *)phys_base_address,
                (uint32_t)PAGE_SIZE);
        return NULL;
    }

    const struct range mmio_range =
        RANGE_INIT(phys_base_address, PAGE_SIZE);
    struct mmio_region *const mmio =
        vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "madt: failed to mmio-map msi-frame at phys address %p\n",
               (void *)phys_base_address);
        return NULL;
    }

    const struct gic_msi_frame msi_frame = {
        .mmio = mmio,
        .id = array_item_count(g_dist.msi_frame_list),
    };

    assert_msg(array_append(&g_dist.msi_frame_list, &msi_frame),
               "gic: failed to append msi-frame");

    return array_back(g_dist.msi_frame_list);
}

__optimize(3) const struct gic_distributor *gic_get_dist() {
    assert_msg(g_dist_initialized,
               "gic: gic_get_dist() called before gicd_init()");
    return &g_dist;
}

__optimize(3) void gicd_mask_irq(const uint8_t irq) {
    mmio_write(&g_regs->interrupt_clear_enable[irq / sizeof(uint32_t)],
               1ull << (irq % sizeof(uint32_t)));
}

__optimize(3) void gicd_unmask_irq(const uint8_t irq) {
    mmio_write(&g_regs->interrupt_set_enable[irq / sizeof(uint32_t)],
               1ull << (irq % sizeof(uint32_t)));
}

__optimize(3)
void gicd_set_irq_affinity(const uint8_t irq, const uint8_t iface) {
    const uint8_t index = irq / sizeof(uint32_t);
    const uint32_t target =
        atomic_load_explicit(&g_regs->interrupt_targets[index],
                             memory_order_relaxed);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_CPU_COUNT;
    const uint32_t new_target =
        (target & (uint32_t)~(0xFF << bit_index)) |
        (target | (1ull << (iface + bit_index)));

    atomic_store_explicit(&g_regs->interrupt_targets[index],
                          new_target,
                          memory_order_relaxed);
}

__optimize(3)
void gicd_set_irq_priority(const uint8_t irq, const uint8_t priority) {
    const uint8_t index = irq / sizeof(uint32_t);
    const uint32_t irq_priority =
        atomic_load_explicit(&g_regs->interrupt_priority[index],
                             memory_order_relaxed);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_CPU_COUNT;
    const uint32_t new_priority =
        (irq_priority & (uint32_t)~(0xFF << bit_index)) |
        (irq_priority | (uint32_t)priority << bit_index);

    atomic_store_explicit(&g_regs->interrupt_priority[index],
                          new_priority,
                          memory_order_relaxed);
}

irq_number_t gic_cpu_get_irq_number(const struct cpu_info *const cpu) {
    const uint64_t ack =
        atomic_load_explicit(&cpu->gic_cpu.interface->interrupt_acknowledge,
                             memory_order_relaxed);

    return ack & __GIC_CPU_EOI_IRQ_ID;
}

uint32_t gic_cpu_get_irq_priority(const struct cpu_info *const cpu) {
    return atomic_load_explicit(&cpu->gic_cpu.interface->running_polarity,
                                memory_order_relaxed);
}

void gic_cpu_eoi(const struct cpu_info *const cpu, const irq_number_t number) {
    if (cpu->gic_cpu.mmio->size > PAGE_SIZE) {
        mmio_write(&cpu->gic_cpu.interface->deactivation,
                   (uint32_t)cpu->cpu_interface_number |
                   (uint32_t)number << GIC_CPU_EOI_CPU_ID_SHIFT);
    } else {
        mmio_write(&cpu->gic_cpu.interface->end_of_interrupt,
                   (uint32_t)cpu->cpu_interface_number |
                   (uint32_t)number << GIC_CPU_EOI_CPU_ID_SHIFT);
    }
}

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop *const int_controller_node =
        devicetree_node_get_prop(node, DEVICETREE_PROP_INTERRUPT_CONTROLLER);

    if (int_controller_node == NULL) {
        printk(LOGLEVEL_WARN,
               "gic: dtb-node is missing interrupt-controller property\n");
        return false;
    }

    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN, "gic: dtb-node is missing reg property\n");
        return false;
    }

    if (array_item_count(reg_prop->list) != 2) {
        printk(LOGLEVEL_WARN,
               "gic: reg prop of dtb node is of the incorrect length\n");
        return false;
    }

    struct devicetree_prop_reg_info *const dist_reg_info =
        array_front(reg_prop->list);

    gicd_init(dist_reg_info->address, /*gic_version=*/2);

    const struct devicetree_node *child_node = NULL;
    list_foreach(child_node, &node->child_list, sibling_list) {
        const struct devicetree_prop_compat *const compat_prop =
            (const struct devicetree_prop_compat *)(uint64_t)
                devicetree_node_get_prop(child_node, DEVICETREE_PROP_COMPAT);

        if (compat_prop == NULL) {
            continue;
        }

        if (!devicetree_prop_compat_has_sv(compat_prop,
                                           SV_STATIC("arm,gic-v2m-frame")))
        {
            continue;
        }

        const struct devicetree_prop_no_value *const msi_controller_prop =
            (const struct devicetree_prop_no_value *)(uint64_t)
                devicetree_node_get_prop(child_node,
                                         DEVICETREE_PROP_MSI_CONTROLLER);

        if (msi_controller_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "gic: msi child of interrupt-controller dtb node is missing "
                   "msi-controller property\n");

            return true;
        }

        const struct devicetree_prop_reg *const msi_reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(child_node, DEVICETREE_PROP_REG);

        if (msi_reg_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "gic: msi dtb-node is missing reg property\n");

            return false;
        }

        if (array_item_count(msi_reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "gic: reg prop of msi dtb node is of the incorrect "
                   "length\n");

            return false;
        }

        struct devicetree_prop_reg_info *const msi_reg_info =
            array_front(msi_reg_prop->list);

        if (msi_reg_info->size != 0x1000) {
            printk(LOGLEVEL_INFO,
                   "gic: msi-reg of dtb node has a size other than 0x1000\n");
            return false;
        }

        gicd_add_msi(msi_reg_info->address);
    }

    return true;
}

static const struct string_view compat_list[] = {
    SV_STATIC("arm,gic-400"), SV_STATIC("arm,cortex-a15-gic")
};

static const struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat_list,
    .compat_count = countof(compat_list),
};

__driver static const struct driver driver = {
    .name = "gicv2-driver",
    .dtb = &dtb_driver,
    .pci = NULL
};