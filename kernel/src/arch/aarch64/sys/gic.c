/*
 * kernel/arch/aarch64/sys/gic.c
 * © suhas pai
 */

#include "acpi/structs.h"
#include "dev/printk.h"
#include "lib/align.h"
#include "mm/mmio.h"

#include "cpu.h"
#include "gic.h"
#include "sys/mmio.h"

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

    volatile uint32_t interrupt_priority[256];
    volatile uint32_t interrupt_targets[256];

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
};

struct gic_cpu_interface {
    volatile uint32_t interrupt_control;
    volatile uint32_t priority_mask;
    volatile uint32_t binary_point;
    volatile uint32_t interrupt_acknowledge;
    volatile uint32_t end_of_interrupt;
};

_Static_assert(sizeof(struct gicd_registers) == 0x10000,
               "struct gicd_registers has an incorrect size");

static struct gic_distributor g_dist = {
    .hardware_id = 0,
    .sys_vector_base = 0,
    .msi_frame_list = ARRAY_INIT(sizeof(struct gic_msi_frame)),
    .version = GICv1,
    .impl_cpu_count = 0,
    .max_impl_lockable_spis = 0
};

static struct mmio_region *g_mmio = NULL;
static volatile struct gicd_registers *g_regs = NULL;
static bool g_dist_initialized = false;

static void init_with_regs() {
    mmio_write(&g_regs->control, /*value=*/0);

    // Enable all global interrupts to be distributed to any cpu.
    uint8_t mask = 0;
    struct cpu_info *cpu = NULL;

    list_foreach(cpu, &g_cpu_list, cpu_list) {
        mask |= 1ull << cpu->mpidr;
    }

    const uint32_t mask_four_times =
        (uint32_t)mask << 24 |
        (uint32_t)mask << 16 |
        (uint32_t)mask << 8 |
        mask;

    for (uint16_t i = GIC_SPI_INTERRUPT_START;
         i < g_dist.interrupt_lines_count;
         i += 4)
    {
        const uint16_t index = i / sizeof(uint32_t);

        mmio_write(&g_regs->interrupt_priority[index], /*value=*/0);
        mmio_write(&g_regs->interrupt_targets[index],
                   /*value=*/mask_four_times);
    }

    for (uint16_t i = GIC_SPI_INTERRUPT_START;
         i < g_dist.interrupt_lines_count;
         i += 16)
    {
        const uint16_t index = i / (4 * sizeof(uint32_t));
        mmio_write(&g_regs->interrupt_config[index], /*value=*/0);
    }

    for (uint16_t i = GIC_SPI_INTERRUPT_START;
         i < g_dist.interrupt_lines_count;
         i += 32)
    {
        const uint16_t index = i / (sizeof(uint32_t) * 8);
        mmio_write(&g_regs->interrupt_group[index], /*value=*/0);
    }

    if (g_dist.version != GICv1) {
        for (uint16_t i = GIC_SPI_INTERRUPT_START;
             i <  g_dist.interrupt_lines_count;
             i += 32)
        {
            const uint32_t index = i / (2 * sizeof(uint32_t));
            mmio_write(g_regs->interrupt_clear_enable + index, 0xFFFFFFFF);
        }
    } else {
        for (uint16_t i = GIC_SPI_INTERRUPT_START;
             i <  g_dist.interrupt_lines_count;
             i += 32)
        {
            const uint32_t index = i / (2 * sizeof(uint32_t));
            mmio_write(g_regs->interrupt_clear_active_enable + index,
                       0xFFFFFFFF);
        }
    }

    mmio_write(&g_regs->control, 1);
    printk(LOGLEVEL_INFO, "gic: finished initializing gicd\n");
}

void gic_cpu_init(volatile struct gic_cpu_interface *const intr) {
    if (g_dist.version == GICv1) {
        mmio_write(g_regs->interrupt_clear_active_enable, 0xFFFFFFFF);
    }

    mmio_write(&g_regs->interrupt_clear_enable[0], 0xFFFF0000);
    mmio_write(&g_regs->interrupt_set_enable[0], 0x0000FFFF);

    for (uint16_t i = 0; i < 32; i += 4) {
        const uint16_t index = i / 4;
        mmio_write(&g_regs->interrupt_priority[index], 0xa0a0a0a0);
    }

    mmio_write(&intr->priority_mask, 0xF0);

    uint32_t interrupt_control = mmio_read(&intr->interrupt_control);
    if (g_dist.version >= GICv2) {
        interrupt_control &= (uint32_t)~__GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE;
    }

    interrupt_control |= __GIC_CPU_INTR_CTRL_ENABLE;
    mmio_write(&intr->interrupt_control, interrupt_control);

    printk(LOGLEVEL_INFO, "gic: initialized cpu interface\n");
}

void gic_dist_init(const struct acpi_madt_entry_gic_distributor *const dist) {
    if (g_dist_initialized) {
        printk(LOGLEVEL_WARN,
               "gic: attempting to initialize multiple gic distributions\n");
        return;
    }

    struct range mmio_range =
        RANGE_INIT(dist->phys_base_address, sizeof(struct gicd_registers));

    if (!range_align_out(mmio_range, PAGE_SIZE, &mmio_range)) {
        printk(LOGLEVEL_WARN,
               "gic: failed to align gicd register range to page-size\n");
        return;
    }

    g_mmio = vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_mmio == NULL) {
        printk(LOGLEVEL_WARN, "gic: failed to mmio-map dist registers\n");
        return;
    }

    g_regs = g_mmio->base;

    g_dist.hardware_id = dist->gic_hardware_id;
    g_dist.sys_vector_base = dist->sys_vector_base;
    g_dist.version = dist->gic_version;

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
           "\thardware-id: %" PRIu32 "\n"
           "\tSystem Vector Base: %" PRIu32 "\n"
           "\tVersion: %" PRIu8 "\n"
           "\tInterrupt Line Count: %" PRIu16 "\n"
           "\tImplemented CPU count: %" PRIu32 "\n"
           "\tMax Implemented Lockable Sets: %" PRIu32 "\n"
           "\tSupports Security Extensions: %s\n",
           g_dist.hardware_id,
           g_dist.sys_vector_base,
           g_dist.version,
           g_dist.interrupt_lines_count,
           g_dist.impl_cpu_count,
           g_dist.max_impl_lockable_spis,
           g_dist.supports_security_extensions ? "yes" : "no");

    init_with_regs();
    g_dist_initialized = true;
}

struct gic_msi_frame *
gic_dist_add_msi(const struct acpi_madt_entry_gic_msi_frame *const frame) {
    if (!has_align(frame->phys_base_address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
                "madt: gic msi frame's physical base address (%p) is not "
                "aligned to the page-size (%" PRIu32 ")\n",
                (void *)frame->phys_base_address,
                (uint32_t)PAGE_SIZE);
        return NULL;
    }

    const struct range mmio_range =
        RANGE_INIT(frame->phys_base_address, PAGE_SIZE);
    struct mmio_region *const mmio =
        vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "madt: failed to mmio-map msi-frame at phys address %p\n",
               (void *)frame->phys_base_address);
        return NULL;
    }

    const struct gic_msi_frame msi_frame = {
        .mmio = mmio,
        .id = frame->msi_frame_id,
        .spi_base = frame->spi_base,
        .spi_count = frame->spi_count,
        .use_msi_typer =
            frame->flags & __ACPI_MADT_GICMSI_FRAME_OVERR_MSI_TYPERR
    };

    assert_msg(array_append(&g_dist.msi_frame_list, &msi_frame),
               "gic: failed to append msi-frame");

    return array_back(g_dist.msi_frame_list);
}

__optimize(3) const struct gic_distributor *get_gic_dist() {
    assert_msg(__builtin_expect(g_dist_initialized, 1),
               "gic: get_gic_dist() called before gic_dist_init()");
    return &g_dist;
}