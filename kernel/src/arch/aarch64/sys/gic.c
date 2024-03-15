/*
 * kernel/src/arch/aarch64/sys/gic.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "cpu/info.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "lib/bits.h"

#include "mm/kmalloc.h"
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

struct gicd_v2_registers {
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
        __GIC_CPU_INTR_CTRL_ENABLE_GROUP_0
        | __GIC_CPU_INTR_CTRL_ENABLE_GROUP_1,

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

_Static_assert(sizeof(struct gicd_v2_registers) == 0x10000,
               "struct gicd_registers has an incorrect size");

#define GICD_CPU_COUNT 8
#define GICD_BITS_PER_IFACE 8
#define GICD_DEFAULT_PRIO 0xA0

static struct gic_distributor g_dist = {
    .msi_frame_list = ARRAY_INIT(sizeof(struct gic_msi_frame)),
    .version = GICv1,
    .impl_cpu_count = 0,
    .max_impl_lockable_spis = 0
};

static struct mmio_region *g_dist_mmio = NULL;
static volatile struct gicd_v2_registers *g_regs = NULL;

static struct mmio_region *g_cpu_mmio = NULL;
static volatile struct gic_cpu_interface *g_cpu = NULL;

static struct list g_msi_info_list = LIST_INIT(g_msi_info_list);

static uint64_t g_cpu_phys_addr = 0;
static uint64_t g_cpu_size = 0;

static bool g_dist_initialized = false;
static bool g_use_split_eoi = false;

static void init_with_regs() {
    mmio_write(&g_regs->control, /*value=*/0);

    const bool flag = disable_interrupts_if_not();
    const uint8_t intr_number = this_cpu()->interface_number;

    for (uint16_t irq = GIC_SPI_INTERRUPT_START;
         irq < g_dist.interrupt_lines_count;
         irq++)
    {
        gicd_mask_irq(irq);

        gicd_set_irq_priority(irq, GICD_DEFAULT_PRIO);
        gicd_set_irq_affinity(irq, intr_number);
    }

    mmio_write(&g_regs->control, /*value=*/1);
    enable_interrupts_if_flag(flag);

    printk(LOGLEVEL_INFO, "gic: finished initializing gicd\n");
}

static uint8_t get_cpu_iface_no() {
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

void gic_init_on_this_cpu(const uint64_t phys_addr, const uint64_t size) {
    for (uint8_t irq = GIC_SGI_INTERRUPT_START;
         irq <= GIC_PPI_INTERRUPT_LAST;
         irq++)
    {
        gicd_mask_irq(irq);
        gicd_set_irq_priority(irq, GICD_DEFAULT_PRIO);
    }

    if (g_cpu_mmio != NULL) {
        assert(phys_addr == g_cpu_phys_addr && size == g_cpu_size);
    } else {
        assert_msg(has_align(size, PAGE_SIZE),
                "gic: cpu interface mmio-region isn't aligned to page-size\n");

        g_cpu_mmio =
            vmap_mmio(RANGE_INIT(phys_addr, size),
                      PROT_READ | PROT_WRITE | PROT_DEVICE,
                      /*flags=*/0);

        assert_msg(g_cpu_mmio != NULL,
                   "gic: failed to allocate mmio-region for cpu-interface");

        g_cpu_phys_addr = phys_addr;
        g_cpu_size = size;

        g_cpu = g_cpu_mmio->base;
        g_use_split_eoi = size > PAGE_SIZE;

        if (g_use_split_eoi) {
            printk(LOGLEVEL_INFO, "gic: using split-eoi for cpu-interface\n");
        }

        printk(LOGLEVEL_INFO,
               "gic: cpu interface at %p, size: 0x%" PRIx64 "\n",
               g_cpu,
               size);
    }

    mmio_write(&g_cpu->priority_mask, 0xF0);
    mmio_write(&g_cpu->active_priority_base[0], /*value=*/0);
    mmio_write(&g_cpu->active_priority_base[1], /*value=*/0);
    mmio_write(&g_cpu->active_priority_base[2], /*value=*/0);
    mmio_write(&g_cpu->active_priority_base[3], /*value=*/0);

    const uint32_t bypass =
        mmio_read(&g_cpu->interrupt_control) &
        __GIC_CPU_INTR_CTLR_FIQ_BYPASS_DISABLE;

    mmio_write(&g_cpu->interrupt_control,
                __GIC_CPU_INTR_CTRL_ENABLE |
                bypass
                | (g_use_split_eoi ? __GIC_CPU_INTERFACE_CTRL_SPLIT_EOI : 0));

    printk(LOGLEVEL_INFO,
           "gic: cpu iface no is %" PRIu8 "\n",
           get_cpu_iface_no());

    printk(LOGLEVEL_INFO, "gic: initialized cpu interface\n");
}

void gicd_init(const uint64_t phys_base_address, const uint8_t gic_version) {
    if (g_dist_initialized) {
        printk(LOGLEVEL_WARN,
               "gic: attempting to initialize multiple gic distributions\n");
        return;
    }

    struct range mmio_range =
        RANGE_INIT(phys_base_address, sizeof(struct gicd_v2_registers));

    if (!range_align_out(mmio_range, PAGE_SIZE, &mmio_range)) {
        printk(LOGLEVEL_WARN,
               "gic: failed to align gicd register range to page-size\n");
        return;
    }

    g_dist_mmio = vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_dist_mmio == NULL) {
        printk(LOGLEVEL_WARN, "gic: failed to mmio-map dist registers\n");
        return;
    }

    g_regs = g_dist_mmio->base;
    g_dist.version = gic_version;

    assert_msg(g_dist.version >= GICv1 && g_dist.version <= GIC_VERSION_BACK,
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
           "\tversion: %" PRIu8 "\n"
           "\tinterrupt line count: %" PRIu16 "\n"
           "\timplemented cpu count: %" PRIu32 "\n"
           "\tmax implemented lockable sets: %" PRIu32 "\n"
           "\tsupports security extensions: %s\n",
           g_dist.version,
           g_dist.interrupt_lines_count,
           g_dist.impl_cpu_count,
           g_dist.max_impl_lockable_spis,
           g_dist.supports_security_extensions ? "yes" : "no");

    init_with_regs();
    g_dist_initialized = true;
}

__optimize(3)
volatile uint64_t *gicd_get_msi_address(const isr_vector_t vector) {
    struct gic_v2_msi_info *iter = NULL;
    list_foreach(iter, &g_msi_info_list, list) {
        const struct range spi_range =
            RANGE_INIT(iter->spi_base, iter->spi_count);

        if (range_has_loc(spi_range, vector)) {
            return &iter->regs->setspi_ns;
        }
    }

    return NULL;
}

static
bool init_msi_frame(struct mmio_region *const mmio, const bool init_later) {
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
               "gic: msi-frame is outside of spi interrupt range\n");
        return false;
    }

    if (!init_later) {
        isr_reserve_msi_irqs(spi_base, spi_count);
    }

    struct gic_v2_msi_info *const info = kmalloc(sizeof(*info));
    if (info == NULL) {
        return false;
    }

    info->mmio = mmio;
    info->regs = regs;
    info->spi_base = spi_base;
    info->spi_count = spi_count;
    info->initialized = !init_later;

    list_add(&g_msi_info_list, &info->list);
    return true;
}

void gicd_add_msi(const uint64_t phys_base_address, const bool init_later) {
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

    if (!init_msi_frame(mmio, init_later)) {
        vunmap_mmio(mmio);
        return;
    }

    const struct gic_msi_frame msi_frame = {
        .mmio = mmio,
        .id = array_item_count(g_dist.msi_frame_list),
    };

    assert_msg(array_append(&g_dist.msi_frame_list, &msi_frame),
               "gicd: failed to append msi-frame to list");
}

void gicd_init_all_msi() {
    struct gic_v2_msi_info *iter = NULL;
    list_foreach(iter, &g_msi_info_list, list) {
        if (!iter->initialized) {
            isr_reserve_msi_irqs(iter->spi_base, iter->spi_count);
            iter->initialized = true;
        }
    }
}

__optimize(3) const struct gic_distributor *gic_get_dist() {
    assert_msg(g_dist_initialized,
               "gic: gic_get_dist() called before gicd_init()");
    return &g_dist;
}

__optimize(3) struct list *gicd_get_msi_info_list() {
    return &g_msi_info_list;
}

__optimize(3) void gicd_mask_irq(const irq_number_t irq) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicd_mask_irq() called on invalid interrupt");

    const uint8_t index = irq / sizeof_bits(uint32_t);
    const uint8_t bit_index = irq % sizeof_bits(uint32_t);

    mmio_write(&g_regs->interrupt_clear_enable[index], 1ull << bit_index);
}

__optimize(3) void gicd_unmask_irq(const irq_number_t irq) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicd_unmask_irq() called on invalid interrupt");

    const uint8_t index = irq / sizeof_bits(uint32_t);
    const uint8_t bit_index = irq % sizeof_bits(uint32_t);

    mmio_write(&g_regs->interrupt_set_enable[index], 1ull << bit_index);
}

__optimize(3)
void gicd_set_irq_affinity(const irq_number_t irq, const uint8_t iface) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicd_set_irq_affinity() called on invalid interrupt");

    const uint8_t index = irq / sizeof(uint32_t);
    const uint32_t target =
        atomic_load_explicit(&g_regs->interrupt_targets[index],
                             memory_order_relaxed);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_BITS_PER_IFACE;
    const uint32_t new_target =
        rm_mask(target, (uint32_t)0xFF << bit_index) |
        1ull << (iface + bit_index);

    atomic_store_explicit(&g_regs->interrupt_targets[index],
                          new_target,
                          memory_order_relaxed);
}

__optimize(3) void
gicd_set_irq_trigger_mode(const irq_number_t irq,
                          const enum irq_trigger_mpde mode)
{
    assert_msg(irq > GIC_SGI_INTERRUPT_LAST,
               "gicd_set_irq_trigger_mode() called on sgi interrupt");
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicd_set_irq_trigger_mode() called on invalid interrupt");

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

__optimize(3)
void gicd_set_irq_priority(const irq_number_t irq, const uint8_t priority) {
    assert_msg(irq <= GIC_SPI_INTERRUPT_LAST,
               "gicd_set_irq_priority() called on invalid interrupt");

    const uint16_t index = irq / sizeof(uint32_t);
    const uint32_t irq_priority =
        atomic_load_explicit(&g_regs->interrupt_priority[index],
                             memory_order_relaxed);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_BITS_PER_IFACE;
    const uint32_t new_priority =
        rm_mask(irq_priority, (uint32_t)0xFF << bit_index) |
        (uint32_t)priority << bit_index;

    atomic_store_explicit(&g_regs->interrupt_priority[index],
                          new_priority,
                          memory_order_relaxed);
}

__optimize(3) irq_number_t gic_cpu_get_irq_number(uint8_t *const cpu_id_out) {
    const uint64_t ack =
        atomic_load_explicit(&g_cpu->interrupt_acknowledge,
                             memory_order_relaxed);

    *cpu_id_out = (ack & __GIC_CPU_EOI_CPU_ID) >> GIC_CPU_EOI_CPU_ID_SHIFT;
    return ack & __GIC_CPU_EOI_IRQ_ID;
}

__optimize(3) uint32_t gic_cpu_get_irq_priority() {
    return atomic_load_explicit(&g_cpu->running_polarity, memory_order_relaxed);
}

__optimize(3)
void gic_cpu_eoi(const uint8_t cpu_id, const irq_number_t irq_number) {
    const uint32_t value =
        (uint32_t)cpu_id << GIC_CPU_EOI_CPU_ID_SHIFT | irq_number;

    if (g_use_split_eoi) {
        mmio_write(&g_cpu->deactivation, value);
    } else {
        mmio_write(&g_cpu->end_of_interrupt, value);
    }
}

bool
gic_init_from_dtb(const struct devicetree *const tree,
                  const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop *const intr_controller_node =
        devicetree_node_get_prop(node, DEVICETREE_PROP_INTERRUPT_CONTROLLER);

    if (intr_controller_node == NULL) {
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
    devicetree_node_foreach_child(node, child_node) {
        const struct string_view msi_sv = SV_STATIC("arm,gic-v2m-frame");
        if (!devicetree_node_has_compat_sv(child_node, msi_sv)) {
            continue;
        }

        const struct devicetree_prop_no_value *const msi_controller_prop =
            (const struct devicetree_prop_no_value *)(uint64_t)
                devicetree_node_get_prop(child_node,
                                         DEVICETREE_PROP_MSI_CONTROLLER);

        if (msi_controller_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "gic: msi child of interrupt-controller dtb node is missing "
                   "a msi-controller property\n");

            return true;
        }

        const struct devicetree_prop_reg *const msi_reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(child_node, DEVICETREE_PROP_REG);

        if (msi_reg_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "gic: msi dtb-node is missing 'reg' property\n");

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

        gicd_add_msi(msi_reg_info->address, /*init_later=*/false);
    }

    struct devicetree_prop_reg_info *const cpu_reg_info =
        array_at(reg_prop->list, /*index=*/1);

    gic_init_on_this_cpu(cpu_reg_info->address, cpu_reg_info->size);
    return true;
}