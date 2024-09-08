/*
 * kernel/src/arch/aarch64/sys/gic/v2.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "sys/gic/api.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/bits.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/mmio.h"
#include "sched/thread.h"

#include "v2.h"

#define GIC_DIST_IMPLEMENTER_ID_RESET 0x0001043B

struct gic_msi_frame {
    uint32_t id;
};

struct gic_distributor {
    struct array msi_frame_list;

    uint8_t impl_cpu_count;
    uint8_t max_impl_lockable_spis;

    uint16_t interrupt_lines_count;
    bool supports_security_extensions : 1;
};

struct gicd_v2m_msi_frame_registers {
    volatile const uint64_t reserved;
    volatile uint32_t typer;

    volatile const char reserved_2[52];
    volatile uint64_t setspi_ns;
};

struct gic_v2_msi_info {
    struct list list;
    uint64_t phys_addr;

    uint16_t spi_base;
    uint16_t spi_count;
};

struct gic_cpu_interface;

enum gicdv2_type_shifts {
    GICDV2_CPU_IMPLD_COUNT_MINUS_ONE_SHIFT = 5,
    GICDV2_MAX_IMPLD_LOCKABLE_SPIS_SHIFT = 11
};

enum gicdv2_type_flags {
    __GICDV2_TYPE_INTR_LINES = 0b11111ull,
    __GICDV2_CPU_IMPLD_COUNT_MINUS_ONE =
        0b111ull << GICDV2_CPU_IMPLD_COUNT_MINUS_ONE_SHIFT,

    __GICDV2_IMPLS_SECURITY_EXTENSIONS = 1ull << 10,
    __GICDV2_MAX_IMPLD_LOCKABLE_SPIS =
        0b11111ull << GICDV2_MAX_IMPLD_LOCKABLE_SPIS_SHIFT
};

struct gicdv2_registers {
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
    volatile _Atomic uint32_t interrupt_config[64];

    volatile uint32_t interrupt_group_modifiers[64];
    volatile uint32_t non_secure_access_control[64];

    volatile _Atomic uint32_t software_generated_interrupts[32]; // Write-only
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

enum gicdv2_sgi_shifts {
    GICD_V2_SGI_CPU_TARGET_MASK_SHIFT = 16
};

enum gicdv2_sgi_target_list_filter {
    GICD_V2_SGI_TARGET_LIST_FILTER_USE_FIELD,
    GICD_V2_SGI_TARGET_LIST_FILTER_ALL_OTHER_CPUS,
    GICD_V2_SGI_TARGET_LIST_FILTER_ONLY_SELF_IPI,
};

enum gicd_v2m_msi_frame_setspi_ns_shifts {
    GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_BASE_SHIFT = 16,
};

enum gicd_v2m_msi_frame_setspi_ns_flags {
    __GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_COUNT = 0x3ff,
    __GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_BASE =
        0x3ff << GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_BASE_SHIFT,
};

enum gic_cpu_interrupt_control_flags {
    __GIC_CPU_INTR_CTRL_ENABLE_GROUP_0 = 1ull << 0,
    __GIC_CPU_INTR_CTRL_ENABLE_GROUP_1 = 1ull << 1,

    __GIC_CPU_INTR_CTRL_ENABLE =
        __GIC_CPU_INTR_CTRL_ENABLE_GROUP_0 | __GIC_CPU_INTR_CTRL_ENABLE_GROUP_1,

    // Only valid on GICv2
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_0 = 1ull << 5,
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_1 = 1ull << 6,
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_2 = 1ull << 7,
    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_3 = 1ull << 8,

    __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE =
        __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_0
      | __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_1
      | __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_2
      | __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE_GROUP_3,

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

_Static_assert(sizeof(struct gicdv2_registers) == 0x10000,
               "struct gicdv2_registers has an incorrect size");

#define GICD_CPU_COUNT 8
#define GICD_BITS_PER_IFACE 8
#define GICD_DEFAULT_PRIO 0xA0

static struct gic_distributor g_dist = {
    .msi_frame_list = ARRAY_INIT(sizeof(struct gic_msi_frame)),
    .impl_cpu_count = 0,
    .max_impl_lockable_spis = 0
};

struct irq_info {
    isr_func_t handler;

    bool alloced_in_msi : 1;
    bool for_msi : 1;
};

#define ISR_IRQ_COUNT 1020

static struct irq_info g_irq_info_list[ISR_IRQ_COUNT] = {0};

static struct mmio_region *g_dist_mmio = NULL;
static volatile struct gicdv2_registers *g_regs = NULL;

static struct mmio_region *g_cpu_mmio = NULL;
static volatile struct gic_cpu_interface *g_cpu = NULL;

static struct list g_msi_info_list = LIST_INIT(g_msi_info_list);
static struct range g_cpu_phys_range= RANGE_EMPTY();

static bool g_dist_initialized = false;
static bool g_use_split_eoi = false;

static void init_with_regs() {
    mmio_write(&g_regs->control, /*value=*/0);
    with_preempt_disabled({
        const uint8_t intr_number = this_cpu()->processor_id;
        for (uint16_t irq = GIC_SPI_INTERRUPT_START;
             irq < g_dist.interrupt_lines_count;
             irq++)
        {
            gicdv2_mask_irq(irq);

            gicdv2_set_irq_priority(irq, GICD_DEFAULT_PRIO);
            gicdv2_set_irq_affinity(irq, intr_number);
        }

        mmio_write(&g_regs->control, /*value=*/1);
    });

    printk(LOGLEVEL_INFO, "gicv2: finished initializing gicd\n");
}

static uint8_t get_cpu_iface_number() {
    for (uint8_t i = 0; i != 8; i++) {
        const uint32_t target =
            atomic_load_explicit(&g_regs->interrupt_targets[i],
                                 memory_order_relaxed);

        if (target == 0) {
            continue;
        }

        const uint32_t mask =
            ((target >> 24) & 0xFF)
          | ((target >> 16) & 0xFF)
          | ((target >> 8) & 0xFF)
          | (target & 0xFF);

        return count_lsb_zero_bits(mask, /*start_index=*/0);
    }

    verify_not_reached();
}

void gicv2_init_on_this_cpu() {
    for (uint8_t irq = GIC_SGI_INTERRUPT_START;
         irq <= GIC_PPI_INTERRUPT_LAST;
         irq++)
    {
        gicdv2_mask_irq(irq);
        gicdv2_set_irq_priority(irq, GICD_DEFAULT_PRIO);
    }
}

bool
gicv2_init_from_info(const struct range cpu_range,
                     const uint64_t phys_base_address)
{
    if (g_dist_initialized) {
        printk(LOGLEVEL_WARN,
               "gicv2: attempting to initialize multiple gic distributions\n");
        return false;
    }

    struct range mmio_range =
        RANGE_INIT(phys_base_address, sizeof(struct gicdv2_registers));

    if (!range_align_out(mmio_range, PAGE_SIZE, &mmio_range)) {
        printk(LOGLEVEL_WARN,
               "gicv2: failed to align gicd register range to page-size\n");
        return false;
    }

    g_dist_mmio = vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_dist_mmio == NULL) {
        printk(LOGLEVEL_WARN, "gicv2: failed to mmio-map dist registers\n");
        return false;
    }

    g_regs = g_dist_mmio->base;
    const uint8_t type = mmio_read(&g_regs->interrupt_controller_type);

    g_dist.interrupt_lines_count =
        min(((type & __GICDV2_TYPE_INTR_LINES) + 1) * 32, 1020);
    g_dist.impl_cpu_count =
        ((type & __GICDV2_CPU_IMPLD_COUNT_MINUS_ONE) >>
            GICDV2_CPU_IMPLD_COUNT_MINUS_ONE_SHIFT) + 1;
    g_dist.max_impl_lockable_spis =
        (type & __GICDV2_MAX_IMPLD_LOCKABLE_SPIS) >>
            GICDV2_MAX_IMPLD_LOCKABLE_SPIS_SHIFT;
    g_dist.supports_security_extensions =
        type & __GICDV2_IMPLS_SECURITY_EXTENSIONS;

    printk(LOGLEVEL_INFO,
           "gic initialized\n"
           "\tinterrupt line count: %" PRIu16 "\n"
           "\timplemented cpu count: %" PRIu32 "\n"
           "\tmax implemented lockable sets: %" PRIu32 "\n"
           "\tsupports security extensions: %s\n",
           g_dist.interrupt_lines_count,
           g_dist.impl_cpu_count,
           g_dist.max_impl_lockable_spis,
           g_dist.supports_security_extensions ? "yes" : "no");

    init_with_regs();
    gic_set_version(2);

    assert_msg(range_has_align(cpu_range, PAGE_SIZE),
               "gicv2: cpu interface mmio-region isn't aligned to "
               "page-size\n");

    g_cpu_mmio =
        vmap_mmio(cpu_range, PROT_READ | PROT_WRITE | PROT_DEVICE, /*flags=*/0);

    assert_msg(g_cpu_mmio != NULL,
               "gicv2: failed to allocate mmio-region for cpu-interface");

    g_cpu = g_cpu_mmio->base;
    g_cpu_phys_range = cpu_range;
    g_use_split_eoi = cpu_range.size > PAGE_SIZE;

    if (g_use_split_eoi) {
        printk(LOGLEVEL_INFO, "gicv2: using split-eoi for cpu-interface\n");
    }

    printk(LOGLEVEL_INFO,
           "gicv2: cpu interface at %p, size: 0x%" PRIx64 "\n",
           g_cpu,
           cpu_range.size);

    mmio_write(&g_cpu->priority_mask, 0xF0);
    mmio_write(&g_cpu->active_priority_base[0], /*value=*/0);
    mmio_write(&g_cpu->active_priority_base[1], /*value=*/0);
    mmio_write(&g_cpu->active_priority_base[2], /*value=*/0);
    mmio_write(&g_cpu->active_priority_base[3], /*value=*/0);

    const uint32_t bypass =
        mmio_read(&g_cpu->interrupt_control)
      & __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE;

    mmio_write(&g_cpu->interrupt_control,
               __GIC_CPU_INTR_CTRL_ENABLE
             | bypass
             | (g_use_split_eoi ? __GIC_CPU_INTERFACE_CTRL_SPLIT_EOI : 0));

    printk(LOGLEVEL_INFO,
           "gicv2: cpu iface no is %" PRIu8 "\n",
           get_cpu_iface_number());

    gicv2_init_on_this_cpu();
    printk(LOGLEVEL_INFO, "gicv2: initialized cpu interface\n");

    g_dist_initialized = true;
    return true;
}

__debug_optimize(3)
volatile uint64_t *gicdv2_get_msi_address(const isr_vector_t vector) {
    struct gic_v2_msi_info *iter = NULL;
    list_foreach(iter, &g_msi_info_list, list) {
        const struct range spi_range =
            RANGE_INIT(iter->spi_base, iter->spi_count);

        if (range_has_loc(spi_range, vector)) {
            struct gicd_v2m_msi_frame_registers *const regs =
                (struct gicd_v2m_msi_frame_registers *)iter->phys_addr;

            return &regs->setspi_ns;
        }
    }

    return NULL;
}

__debug_optimize(3) enum isr_msi_support gicdv2_get_msi_support() {
    return !list_empty(&g_msi_info_list)
            ? ISR_MSI_SUPPORT_MSI : ISR_MSI_SUPPORT_NONE;
}

static
bool init_msi_frame(const uint64_t phys_addr, struct mmio_region *const mmio) {
    volatile struct gicd_v2m_msi_frame_registers *const regs = mmio->base;

    const uint32_t typer = mmio_read(&regs->typer);
    const uint16_t spi_count =
        typer & __GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_COUNT;
    const uint16_t spi_base =
        (typer & __GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_BASE) >>
            GICD_V2M_MSI_FRAME_SETSPI_NS_FLAGS_SPI_BASE_SHIFT;

    printk(LOGLEVEL_INFO,
           "gicd: msi-frame:\n"
           "\tspi-base: %" PRIu16 "\n"
           "\tspi-count: %" PRIu16 "\n",
           spi_base,
           spi_count);

    const uint16_t gic_spi_count =
        distance_incl(GIC_SPI_INTERRUPT_START, GIC_SPI_INTERRUPT_LAST);
    const struct range gic_spi_range =
        RANGE_INIT(GIC_SPI_INTERRUPT_START, gic_spi_count);

    const struct range spi_range = RANGE_INIT(spi_base, spi_count);
    if (!range_has(gic_spi_range, spi_range)) {
        printk(LOGLEVEL_WARN,
               "gicv2: msi-frame is outside of spi interrupt range\n");
        return false;
    }

    struct gic_v2_msi_info *const info = kmalloc(sizeof(*info));
    if (info == NULL) {
        return false;
    }

    info->phys_addr = phys_addr;
    info->spi_base = spi_base;
    info->spi_count = spi_count;

    isr_reserve_msi_irqs(spi_base, spi_count);
    list_add(&g_msi_info_list, &info->list);

    return true;
}

void gicv2_add_msi_frame(const uint64_t phys_base_address) {
    if (!has_align(phys_base_address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
                "gicd: msi frame's physical base address %p is not aligned "
                "to the page-size (%" PRIu32 ")\n",
                (void *)phys_base_address,
                (uint32_t)PAGE_SIZE);
        return;
    }

    const struct range mmio_range = RANGE_INIT(phys_base_address, PAGE_SIZE);
    struct mmio_region *const mmio =
        vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN,
               "gicd: failed to mmio-map msi-frame at phys address %p\n",
               (void *)phys_base_address);
        return;
    }

    const bool result = init_msi_frame(phys_base_address, mmio);
    vunmap_mmio(mmio);

    if (!result) {
        return;
    }

    const struct gic_msi_frame msi_frame = {
        .id = array_item_count(g_dist.msi_frame_list),
    };

    assert_msg(array_append(&g_dist.msi_frame_list, &msi_frame),
               "gicd: failed to append msi-frame to list");
}

__debug_optimize(3) isr_vector_t gicdv2_alloc_msi_vector() {
    struct gic_v2_msi_info *iter = NULL;
    list_foreach(iter, &g_msi_info_list, list) {
        const uint16_t end = iter->spi_base + iter->spi_count;
        for (uint16_t irq = iter->spi_base; irq != end; irq++) {
            assert(g_irq_info_list[irq].for_msi);
            if (!g_irq_info_list[irq].alloced_in_msi) {
                g_irq_info_list[irq].alloced_in_msi = true;
                return irq;
            }
        }
    }

    return ISR_INVALID_VECTOR;
}

void gicdv2_free_msi_vector(const isr_vector_t vector) {
    assert(g_irq_info_list[vector].for_msi);
    assert(g_irq_info_list[vector].alloced_in_msi);

    g_irq_info_list[vector].alloced_in_msi = true;
}

__debug_optimize(3) void gicdv2_mask_irq(const irq_number_t irq) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicdv2_mask_irq() called on invalid interrupt");

    const uint8_t index = irq / sizeof_bits(uint32_t);
    const uint8_t bit_index = irq % sizeof_bits(uint32_t);

    mmio_write(&g_regs->interrupt_clear_enable[index], 1ull << bit_index);
}

__debug_optimize(3) void gicdv2_unmask_irq(const irq_number_t irq) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicdv2_unmask_irq() called on invalid interrupt");

    const uint8_t index = irq / sizeof_bits(uint32_t);
    const uint8_t bit_index = irq % sizeof_bits(uint32_t);

    mmio_write(&g_regs->interrupt_set_enable[index], 1ull << bit_index);
}

__debug_optimize(3)
void gicdv2_set_irq_affinity(const irq_number_t irq, const uint8_t affinity) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicdv2_set_irq_affinity() called on invalid interrupt");

    const uint8_t index = irq / sizeof(uint32_t);
    const uint32_t target =
        atomic_load_explicit(&g_regs->interrupt_targets[index],
                             memory_order_relaxed);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_BITS_PER_IFACE;
    const uint32_t new_target =
        rm_mask(target, (uint32_t)0xFF << bit_index)
      | 1ull << (affinity + bit_index);

    atomic_store_explicit(&g_regs->interrupt_targets[index],
                          new_target,
                          memory_order_relaxed);
}

__debug_optimize(3) void
gicdv2_set_irq_trigger_mode(const irq_number_t irq,
                            const enum irq_trigger_mode mode)
{
    assert_msg(irq > GIC_SGI_INTERRUPT_LAST,
               "gicdv2_set_irq_trigger_mode() called on sgi interrupt");
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicdv2_set_irq_trigger_mode() called on invalid interrupt");

    const uint8_t config_index = irq / 16;
    const uint32_t target =
        atomic_load_explicit(&g_regs->interrupt_config[config_index],
                             memory_order_relaxed);

    const uint8_t shift = (irq % 16) * 2;
    uint32_t new_target = rm_mask(target, 0b11ull << shift);

    switch (mode) {
        case IRQ_TRIGGER_MODE_EDGE:
            new_target |= (uint32_t)0b10 << shift;
            goto write;
        case IRQ_TRIGGER_MODE_LEVEL:
            goto write;
        write:
            atomic_store_explicit(&g_regs->interrupt_config[config_index],
                                  new_target,
                                  memory_order_relaxed);
            return;
    }

    verify_not_reached();
}

__debug_optimize(3)
void gicdv2_set_irq_priority(const irq_number_t irq, const uint8_t priority) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicdv2_set_irq_priority() called on invalid interrupt");

    const uint16_t index = irq / sizeof(uint32_t);
    const uint32_t irq_priority =
        atomic_load_explicit(&g_regs->interrupt_priority[index],
                             memory_order_relaxed);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_BITS_PER_IFACE;
    const uint32_t new_priority =
        rm_mask(irq_priority, (uint32_t)0xFF << bit_index)
      | (uint32_t)priority << bit_index;

    atomic_store_explicit(&g_regs->interrupt_priority[index],
                          new_priority,
                          memory_order_relaxed);
}

__debug_optimize(3)
void gicdv2_send_ipi(const struct cpu_info *const cpu, const uint8_t int_no) {
    const uint32_t info =
        GICD_V2_SGI_TARGET_LIST_FILTER_USE_FIELD
      | 1ull << (GICD_V2_SGI_CPU_TARGET_MASK_SHIFT + cpu->processor_id)
      | int_no;

    atomic_store_explicit(&g_regs->software_generated_interrupts[0],
                          info,
                          memory_order_relaxed);
}

__debug_optimize(3) void gicdv2_send_sipi(const uint8_t int_no) {
    atomic_store_explicit(&g_regs->software_generated_interrupts[0],
                          GICD_V2_SGI_TARGET_LIST_FILTER_ONLY_SELF_IPI | int_no,
                          memory_order_relaxed);
}

__debug_optimize(3)
irq_number_t gicv2_cpu_get_irq_number(uint8_t *const cpu_id_out) {
    const uint64_t ack =
        atomic_load_explicit(&g_cpu->interrupt_acknowledge,
                             memory_order_relaxed);

    *cpu_id_out = (ack & __GIC_CPU_EOI_CPU_ID) >> GIC_CPU_EOI_CPU_ID_SHIFT;
    return ack & __GIC_CPU_EOI_IRQ_ID;
}

__debug_optimize(3) uint32_t gicv2_cpu_get_irq_priority() {
    return atomic_load_explicit(&g_cpu->running_polarity, memory_order_relaxed);
}

__debug_optimize(3)
void gicv2_cpu_eoi(const uint8_t cpu_id, const irq_number_t irq_number) {
    const uint32_t value =
        (uint32_t)cpu_id << GIC_CPU_EOI_CPU_ID_SHIFT | irq_number;

    if (g_use_split_eoi) {
        mmio_write(&g_cpu->deactivation, value);
    } else {
        mmio_write(&g_cpu->end_of_interrupt, value);
    }
}

bool
gicv2_init_from_dtb(const struct devicetree *const tree,
                    const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop *const intr_controller_node =
        devicetree_node_get_prop(node, DEVICETREE_PROP_INTR_CONTROLLER);

    if (intr_controller_node == NULL) {
        printk(LOGLEVEL_WARN,
               "gicv2: dtb-node is missing interrupt-controller property\n");
        return false;
    }

    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN, "gicv2: dtb-node is missing reg property\n");
        return false;
    }

    if (array_item_count(reg_prop->list) != 2) {
        printk(LOGLEVEL_WARN,
               "gicv2: reg prop of dtb node is of the incorrect length\n");
        return false;
    }

    struct devicetree_prop_reg_info *const dist_reg_info =
        array_front(reg_prop->list);

    struct range cpu_reg_range = RANGE_EMPTY();
    struct devicetree_prop_reg_info *const cpu_reg_info =
        array_at(reg_prop->list, /*index=*/1);

    if (!range_create_and_verify(cpu_reg_info->address,
                                 cpu_reg_info->size,
                                 &cpu_reg_range))
    {
        printk(LOGLEVEL_INFO, "gicv2: dtb node's 'reg' prop range overflows\n");
        return false;
    }

    gicv2_init_from_info(cpu_reg_range, dist_reg_info->address);
    devicetree_node_foreach_child(node, child_node) {
        const struct string_view msi_sv = SV_STATIC("arm,gic-v2m-frame");
        if (!devicetree_node_has_compat_sv(child_node, msi_sv)) {
            continue;
        }

        const struct devicetree_prop *const msi_controller_prop =
            devicetree_node_get_prop(child_node,
                                     DEVICETREE_PROP_MSI_CONTROLLER);

        if (msi_controller_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "gicv2: msi child of interrupt-controller dtb node is "
                   "missing a msi-controller property\n");

            return true;
        }

        const struct devicetree_prop_reg *const msi_reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(child_node, DEVICETREE_PROP_REG);

        if (msi_reg_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "gicv2: msi dtb-node is missing a 'reg' property\n");

            return false;
        }

        if (array_item_count(msi_reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "gicv2: reg prop of msi dtb node is of the incorrect "
                   "length\n");

            return false;
        }

        struct devicetree_prop_reg_info *const msi_reg_info =
            array_front(msi_reg_prop->list);

        if (msi_reg_info->size != 0x1000) {
            printk(LOGLEVEL_INFO,
                   "gicv2: msi-reg of dtb node has a size other than 0x1000\n");
            return false;
        }

        gicv2_add_msi_frame(msi_reg_info->address);
    }

    return true;
}