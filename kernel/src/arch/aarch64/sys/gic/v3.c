/*
 * kernel/src/arch/aarch64/sys/gic/v3.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"
#include "asm/sync.h"

#include "cpu/isr.h"
#include "dev/printk.h"

#include "mm/mmio.h"
#include "mm/page_alloc.h"

#include "sys/mmio.h"

#include "api.h"
#include "its.h"

enum gicv3_control_flags {
    __GICDV3_CTRL_ENABLE_GROUP_0 = 1ull << 0,
    __GICDV3_CTRL_ENABLE_GROUP_1_NON_SECURE = 1ull << 1,
    __GICDV3_CTRL_ENABLE_GROUP_1_SECURE = 1ull << 2,

    __GICDV3_CTRL_AFFINITY_ROUTING_ENABLE_SECURE = 1ull << 4,
    __GICDV3_CTRL_AFFINITY_ROUTING_ENABLE_NON_SECURE = 1ull << 5,

    __GICDV3_CTRL_DISABLE_SECURITY = 1ull << 6,
    __GICDV3_CTRL_RWP = 1ull << 30,
};

struct gicd_v3_registers {
    volatile uint32_t control;
    volatile uint32_t type;

    volatile const uint8_t reserved[184];
    volatile _Atomic uint32_t irq_group[16];

    volatile _Atomic uint32_t irq_set_enable[32];
    volatile _Atomic uint32_t irq_clear_enable[32];

    volatile const uint8_t reserved_2[512];
    volatile _Atomic uint32_t irq_priority[32];

    volatile const uint8_t reserved_3[1920];

    volatile _Atomic uint32_t irq_config[64];
    volatile _Atomic uint32_t irq_group_mod[64];

    volatile const uint8_t reserved_4[20990];
    volatile _Atomic uint32_t irq_router[32];
};

enum gicv3_redist_control_flags {
    __GICV3_REDIST_CONTROL_ENABLE_LPIS = 1ull << 0,
    __GICV3_REDIST_CONTROL_RWP = 1ull << 3,
};

enum gicv3_redist_typer_shifts {
    GICV3_REDIST_TYPER_PROCESSOR_NUMBER_SHIFT = 8,
};

enum gicv3_redist_typer_flags {
    __GICV3_REDIST_TYPER_SUPPORTS_PHYS_LPIS = 1ull << 0,
    __GICV3_REDIST_TYPER_LAST = 1ull << 4,
    __GICV3_REDIST_TYPER_AFFINITY = 0xFFFFFFFFull << 32,
    __GICV3_REDIST_TYPER_PROCESSOR_NUMBER =
        0xFFFFull << GICV3_REDIST_TYPER_PROCESSOR_NUMBER_SHIFT,
};

enum gicv3_redist_waker_flags {
    __GICV3_REDIST_WAKER_PROCESSOR_ASLEEP = 1ull << 1,
    __GICV3_REDIST_WAKER_CHILDREN_ASLEEP = 1ull << 2,
};

enum gicv3_redist_prop_baser_shifts {
    GICV3_REDIST_PROP_BASER_PHYS_ADDR_SHIFT = 12
};

enum gicv3_redist_pend_baser_shifts {
    GICV3_REDIST_PEND_BASER_PHYS_ADDR_SHIFT = 16
};

enum gicv3_redist_prop_baser_flags {
    __GICV3_REDIST_PROP_BASER_PHYS_ADDR =
        mask_for_n_bits(40) << GICV3_REDIST_PROP_BASER_PHYS_ADDR_SHIFT
};

enum gicv3_redist_pend_baser_flags {
    __GICV3_REDIST_PEND_BASER_PHYS_ADDR =
        mask_for_n_bits(36) << GICV3_REDIST_PEND_BASER_PHYS_ADDR_SHIFT
};

struct gicv3_redist_registers {
    volatile _Atomic uint32_t control;
    volatile const uint32_t impl_id;

    volatile uint64_t typer;
    volatile uint32_t statusr;
    volatile uint32_t waker;

    volatile const char reserved[88];

    volatile _Atomic uint64_t prop_baser;
    volatile _Atomic uint64_t pend_baser;

    volatile const char reserved_2[65405];
    volatile struct gicd_v3_registers dist;
};

enum icc_sre_flags {
    __ICC_SRE_ENABLE = 1 << 0,
    __ICC_SRE_DISABLE_IRQ_BYPASS = 1 << 1,
    __ICC_SRE_DISABLE_FIQ_BYPASS = 1 << 2
};

enum icc_ctlr_flags {
    __ICC_CTLR_EOI_DEACTIVATE = 1 << 1
};

enum icc_sgi1r_routing_mode {
    ICC_SGI1R_ROUTING_MODE_TARGET_LIST,
    ICC_SGI1R_ROUTING_MODE_ALL_BUT_SELF,
};

enum icc_sgi1r_shifts {
    ICC_SGI1R_AFF1_SHIFT = 16,
    ICC_SGI1R_INTR_ID_SHIFT = 24,
    ICC_SGI1R_AFF2_SHIFT = 32,
    ICC_SGI1R_ROUTING_MODE_SHIFT = 40,
    ICC_SGI1R_AFF3_SHIFT = 48,
};

enum icc_sgi1r_flags {
    __ICC_SGI1R_TARGET_LIST = 0xFFFFull,
    __ICC_SGI1R_AFF1 = 0xFFull << ICC_SGI1R_AFF1_SHIFT,
    __ICC_SGI1R_INTR_ID = 0xFull << ICC_SGI1R_AFF1_SHIFT,
    __ICC_SGI1R_AFF2 = 0xFFull << ICC_SGI1R_AFF2_SHIFT,
    __ICC_SGI1R_ROUTING_MODE = 0xFFull << ICC_SGI1R_ROUTING_MODE_SHIFT,
    __ICC_SGI1R_AFF3 = 0xFFull << ICC_SGI1R_AFF3_SHIFT,
};

volatile static struct gicd_v3_registers *g_dist_regs = NULL;

static struct mmio_region *g_dist_mmio = NULL;
static struct mmio_region *g_redist_mmio = NULL;

static bool gic_initialized = false;
static uint32_t g_redist_count = 0;

#define MAX_ATTEMPTS 100
#define GICD_BITS_PER_IFACE 8
#define GICD_DEFAULT_PRIO 0xA0
#define GIC_REDIST_IDBITS 16

#define GIC_REDIST_PROP_ALLOC_ORDER 1
#define GIC_REDIST_PENDING_ALLOC_ORDER 4

_Static_assert((PAGE_SIZE << GIC_REDIST_PROP_ALLOC_ORDER)
               >= (GIC_ITS_MAX_LPIS_SUPPORTED * sizeof(uint8_t)),
               "GIC_REDIST_PROP_ALLOC_ORDER is too low");

_Static_assert((PAGE_SIZE << GIC_REDIST_PENDING_ALLOC_ORDER)
               >= (GIC_ITS_MAX_LPIS_SUPPORTED * sizeof(uint64_t)),
               "GIC_REDIST_PENDING_ALLOC_ORDER is too low");

__debug_optimize(3)
volatile struct gicv3_redist_registers *gic_redist_for_this_cpu() {
    volatile struct gicv3_redist_registers *redist = g_redist_mmio->base;
    volatile const struct gicv3_redist_registers *const end =
        redist + g_redist_count;

    const uint32_t affinity = this_cpu()->affinity;
    for (; redist != end; redist++) {
        const uint32_t redist_affinity =
            mmio_read(&redist->typer) & __GICV3_REDIST_TYPER_AFFINITY;

        if (redist_affinity == affinity) {
            return redist;
        }
    }

    verify_not_reached();
}

static
volatile struct gicd_v3_registers *gicv3_dist_for_irq(const irq_number_t irq) {
    if (irq < GIC_SPI_INTERRUPT_START) {
        volatile struct gicv3_redist_registers *const redist_regs =
            g_redist_mmio->base;

        return &redist_regs->dist;
    }

    return g_dist_regs;
}

__debug_optimize(3) void gicdv3_mask_irq(const irq_number_t irq) {
    const bool flag = disable_irqs_if_enabled();
    volatile struct gicd_v3_registers *const dist = gicv3_dist_for_irq(irq);

    atomic_store_explicit(&dist->irq_clear_enable[irq / sizeof_bits(uint32_t)],
                          1ull << (irq % sizeof_bits(uint32_t)),
                          memory_order_relaxed);

    enable_irqs_if_flag(flag);
}

__debug_optimize(3) void gicdv3_unmask_irq(const irq_number_t irq) {
    const bool flag = disable_irqs_if_enabled();
    volatile struct gicd_v3_registers *const dist = gicv3_dist_for_irq(irq);

    atomic_store_explicit(&dist->irq_set_enable[irq / sizeof_bits(uint32_t)],
                          1ull << (irq % sizeof_bits(uint32_t)),
                          memory_order_relaxed);

    enable_irqs_if_flag(flag);
}

__debug_optimize(3) isr_vector_t
gicdv3_alloc_msi_vector(struct device *const device, const uint16_t msi_index) {
    struct gic_its_info *its = NULL;
    list_foreach(its, gic_its_get_list(), list) {
        const isr_vector_t vector =
            gic_its_alloc_msi_vector(its, device, msi_index);

        return vector;
    }

    return ISR_INVALID_VECTOR;
}

__debug_optimize(3) void
gicdv3_free_msi_vector(struct device *const device,
                       const isr_vector_t vector,
                       const uint16_t msi_index)
{
    struct gic_its_info *its = NULL;
    list_foreach(its, gic_its_get_list(), list) {
        gic_its_free_msi_vector(its, device, vector, msi_index);
    }
}

__debug_optimize(3)
void gicdv3_set_irq_affinity(const irq_number_t irq, const uint8_t affinity) {
    if (irq < GIC_SPI_INTERRUPT_START) {
        printk(LOGLEVEL_WARN,
               "gicv3: gicd_set_irq_affinity() called on sgi/ppi "
               "interrupt " IRQ_NUMBER_FMT "\n",
               irq);

        return;
    }

    mmio_write(&g_dist_regs->irq_router[irq - GIC_SPI_INTERRUPT_START],
               (uint32_t)affinity << 24
             | (uint32_t)affinity << 16
             | (uint32_t)affinity << 8
             | affinity);
}

__debug_optimize(3) void
gicdv3_set_irq_trigger_mode(const irq_number_t irq,
                            const enum irq_trigger_mode mode)
{
    if (irq < GIC_PPI_INTERRUPT_START) {
        printk(LOGLEVEL_WARN,
               "gicdv3_set_irq_trigger_mode() called on sgi interrupt\n");
        return;
    }

    const bool flag = disable_irqs_if_enabled();
    volatile struct gicd_v3_registers *const dist = gicv3_dist_for_irq(irq);

    const uint32_t bit_offset = (irq % sizeof_bits(uint16_t)) * 2;
    const uint32_t bit_value = mode == IRQ_TRIGGER_MODE_EDGE ? 0b10 : 0b00;
    const uint32_t irq_config =
        atomic_load_explicit(&dist->irq_config[irq / sizeof_bits(uint16_t)],
                             memory_order_relaxed);

    atomic_store_explicit(&dist->irq_config[irq / sizeof_bits(uint16_t)],
                          rm_mask(irq_config, 0b11 << bit_offset)
                        | bit_value << bit_offset,
                          memory_order_relaxed);

    const uint32_t group_bit_offset = irq % sizeof_bits(uint32_t);
    const uint32_t irq_group =
        atomic_load_explicit(&dist->irq_group[irq / sizeof_bits(uint32_t)],
                             memory_order_relaxed);

    atomic_store_explicit(&dist->irq_group[irq / sizeof_bits(uint32_t)],
                          irq_group | 1ull << group_bit_offset,
                          memory_order_relaxed);

    const uint32_t group_mod =
        atomic_load_explicit(&dist->irq_group_mod[irq / sizeof_bits(uint32_t)],
                             memory_order_relaxed);

    atomic_store_explicit(&dist->irq_group_mod[irq / sizeof_bits(uint32_t)],
                          rm_mask(group_mod, 1ull << group_bit_offset),
                          memory_order_relaxed);

    enable_irqs_if_flag(flag);
}

__debug_optimize(3)
void gicdv3_set_irq_priority(const irq_number_t irq, const uint8_t priority) {
    const bool flag = disable_irqs_if_enabled();
    volatile struct gicd_v3_registers *const dist = gicv3_dist_for_irq(irq);

    const uint8_t bit_index = (irq % sizeof(uint32_t)) * GICD_BITS_PER_IFACE;
    const uint32_t irq_priority =
        atomic_load_explicit(&dist->irq_priority[irq], memory_order_relaxed);

    atomic_store_explicit(&dist->irq_priority[irq],
                          rm_mask(irq_priority, 0xFFull << bit_index)
                        | (uint32_t)priority << bit_index,
                          memory_order_relaxed);

    enable_irqs_if_flag(flag);
}

__debug_optimize(3)
void gicdv3_send_ipi(const struct cpu_info *const cpu, const uint8_t int_no) {
    const uint64_t value =
        (uint64_t)(uint8_t)(cpu->affinity >> 24) << ICC_SGI1R_AFF3_SHIFT
      | (uint64_t)(uint8_t)(cpu->affinity >> 16) << ICC_SGI1R_AFF2_SHIFT
      | (uint64_t)(uint8_t)(cpu->affinity >> 8) << ICC_SGI1R_AFF1_SHIFT
      | (uint64_t)int_no << ICC_SGI1R_INTR_ID_SHIFT
      | 1 << (uint8_t)cpu->affinity;

    dsbisht();
    asm volatile("msr icc_sgi1r_el1, %0" :: "r"(value));
    isb();
}

__debug_optimize(3) void gicdv3_send_sipi(const uint8_t int_no) {
    preempt_disable();
    const struct cpu_info *const cpu = this_cpu();

    gicdv3_send_ipi(cpu, int_no);
    preempt_enable();
}

__debug_optimize(3)
volatile uint64_t *gicdv3_get_msi_address(const isr_vector_t vector) {
    (void)vector;

    struct gic_its_info *its = NULL;
    list_foreach(its, gic_its_get_list(), list) {
        volatile uint64_t *const address = gic_its_get_msi_address(its);
        return address;
    }

    return NULL;
}

__debug_optimize(3) enum isr_msi_support gicdv3_get_msi_support() {
    return ISR_MSI_SUPPORT_MSIX;
}

__debug_optimize(3) irq_number_t
gicv3_cpu_get_irq_number(uint8_t *const cpu_id_out) {
    uint64_t iar1 = 0;
    asm volatile("mrs %0, icc_iar1_el1" : "=r"(iar1));

    const uint64_t irq = iar1 & 0xFFFFFF;
    if (irq < GIC_SPI_INTERRUPT_START || irq >= GIC_ITS_LPI_INTERRUPT_START) {
        asm volatile("msr icc_eoir1_el1, %0" :: "r"(irq));
    }

    *cpu_id_out = 0;
    return (irq_number_t)irq;
}

__debug_optimize(3) uint32_t gicv3_cpu_get_irq_priority() {
    return 0;
}

__debug_optimize(3)
void gicv3_cpu_eoi(const uint8_t cpu_id, const irq_number_t irq) {
    (void)cpu_id;

    struct cpu_info *const cpu = this_cpu_mut();
    if (cpu->in_lpi) {
        cpu->in_lpi = false;
    } else {
        asm volatile("msr icc_dir_el1, %0" :: "r"((uint64_t)irq));
    }
}

void gic_redist_init_on_this_cpu() {
    const bool flag = disable_irqs_if_enabled();
    volatile struct gicv3_redist_registers *const redist =
        gic_redist_for_this_cpu();

    const uint64_t typer = mmio_read(&redist->typer);

    assert(typer & __GICV3_REDIST_TYPER_SUPPORTS_PHYS_LPIS);
    mmio_write(&redist->waker,
               rm_mask(mmio_read(&redist->waker),
                       __GICV3_REDIST_WAKER_PROCESSOR_ASLEEP));

    bool cleared = false;
    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&redist->waker)
                & __GICV3_REDIST_WAKER_CHILDREN_ASLEEP) == 0)
        {
            cleared = true;
            break;
        }

        cpu_pause();
    }

    if (!cleared) {
        printk(LOGLEVEL_WARN, "gicv3: child-asleep bit failed to clear\n");
        return;
    }

    mmio_write(&redist->dist.irq_group[0], UINT32_MAX);
    mmio_write(&redist->dist.irq_group_mod[0], 0);
    mmio_write(&redist->control,
               mmio_read(&redist->control)
             | __GICV3_REDIST_CONTROL_ENABLE_LPIS);

    // Pending page has to be aligned to 64kib
    struct page *const pend_page =
        alloc_pages_at_align(PAGE_STATE_USED,
                             __ALLOC_ZERO,
                             /*align=*/16,
                             GIC_REDIST_PENDING_ALLOC_ORDER);

    assert_msg(pend_page != NULL,
               "gicv3: failed to alloc pending page for redistributor\n");

    struct page *const prop_page =
        alloc_pages(PAGE_STATE_USED, __ALLOC_ZERO, GIC_REDIST_PROP_ALLOC_ORDER);

    assert_msg(prop_page != NULL,
               "gicv3: failed to alloc pending page for redistributor\n");

    const uint64_t pend_phys = page_to_phys(pend_page);
    const uint64_t prop_phys = page_to_phys(prop_page);

    mmio_write(&redist->pend_baser, mmio_read(&redist->pend_baser) | pend_phys);
    mmio_write(&redist->prop_baser,
               mmio_read(&redist->prop_baser)
             | prop_phys
             | (GIC_REDIST_IDBITS - 1));

    this_cpu_mut()->processor_id =
        (typer & __GICV3_REDIST_TYPER_PROCESSOR_NUMBER)
            >> GICV3_REDIST_TYPER_PROCESSOR_NUMBER_SHIFT;

    this_cpu_mut()->gic_its_pend_page = phys_to_virt(pend_phys);
    this_cpu_mut()->gic_its_prop_page = phys_to_virt(prop_phys);

    enable_irqs_if_flag(flag);
}

void gicv3_init_on_this_cpu() {
    uint64_t icc_sre = 0;

    asm volatile("mrs %0, icc_sre_el1" : "=r"(icc_sre));
    asm volatile("msr icc_sre_el1, %0" :: "r"(icc_sre | __ICC_SRE_ENABLE));

    isb();
    uint64_t icc_ctlr = 0;

    asm volatile("mrs %0, icc_ctlr_el1" : "=r"(icc_ctlr));
    asm volatile("msr icc_ctlr_el1, %0"
                 :: "r"(icc_ctlr | __ICC_CTLR_EOI_DEACTIVATE));

    asm volatile("msr icc_pmr_el1, %0" :: "r"(0xFFull));
    asm volatile("msr icc_bpr1_el1, %0" :: "r"(0b111ull));

    uint64_t icc_grpen1 = 0;

    asm volatile("mrs %0, icc_igrpen1_el1" : "=r"(icc_grpen1));
    asm volatile("msr icc_igrpen1_el1, %0" :: "r"(icc_grpen1 | 1));

    isb();
    dsb();

    for (irq_number_t irq = GIC_SGI_INTERRUPT_START;
         irq <= GIC_SGI_INTERRUPT_LAST;
         irq++)
    {
        gicdv3_mask_irq(irq);
        gicdv3_set_irq_priority(irq, GICD_DEFAULT_PRIO);
        gicdv3_unmask_irq(irq);
    }

    for (irq_number_t irq = GIC_PPI_INTERRUPT_START;
         irq <= GIC_PPI_INTERRUPT_LAST;
         irq++)
    {
        gicdv3_mask_irq(irq);
        gicdv3_set_irq_priority(irq, GICD_DEFAULT_PRIO);
    }
}

static bool init_from_regs() {
    mmio_write(&g_dist_regs->control,
               __GICDV3_CTRL_AFFINITY_ROUTING_ENABLE_SECURE
               | __GICDV3_CTRL_AFFINITY_ROUTING_ENABLE_NON_SECURE);

    bool rwp_cleared = false;
    for (int i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&g_dist_regs->control) & __GICDV3_CTRL_RWP) == 0) {
            rwp_cleared = true;
            break;
        }

        cpu_pause();
    }

    if (!rwp_cleared) {
        vunmap_mmio(g_dist_mmio);
        vunmap_mmio(g_redist_mmio);

        printk(LOGLEVEL_WARN, "gicv3: rwp bit failed to clear\n");
        return false;
    }

    mmio_write(&g_dist_regs->control,
               __GICDV3_CTRL_ENABLE_GROUP_0
             | __GICDV3_CTRL_ENABLE_GROUP_1_NON_SECURE
             | __GICDV3_CTRL_ENABLE_GROUP_1_SECURE
             | __GICDV3_CTRL_AFFINITY_ROUTING_ENABLE_SECURE
             | __GICDV3_CTRL_AFFINITY_ROUTING_ENABLE_NON_SECURE
             | __GICDV3_CTRL_DISABLE_SECURITY);

    gic_redist_init_on_this_cpu();
    gicv3_init_on_this_cpu();

    return true;
}

bool
gicv3_init_from_info(const uint64_t dist_phys, const struct range redist_range)
{
    if (gic_initialized) {
        printk(LOGLEVEL_WARN,
               "gicv3: attempting to initialize multiple gics\n");
        return false;
    }

    struct range mmio_range =
        RANGE_INIT(dist_phys, sizeof(struct gicd_v3_registers));

    if (!range_align_out(mmio_range, PAGE_SIZE, &mmio_range)) {
        printk(LOGLEVEL_WARN,
               "gic: failed to align gicd register range to page-size\n");
        return false;
    }

    g_dist_mmio = vmap_mmio(mmio_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_dist_mmio == NULL) {
        printk(LOGLEVEL_WARN, "gic: failed to mmio-map dist registers\n");
        return false;
    }

    g_redist_mmio =
        vmap_mmio(redist_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (g_redist_mmio == NULL) {
        printk(LOGLEVEL_WARN, "gic: failed to mmio-map redist range\n");
        vunmap_mmio(g_dist_mmio);

        return false;
    }

    g_dist_regs = g_dist_mmio->base;
    g_redist_count = 1;

    if (!init_from_regs()) {
        vunmap_mmio(g_dist_mmio);
        return false;
    }

    gic_set_version(3);
    gic_initialized = true;

    printk(LOGLEVEL_WARN, "gicv3: fully initialized\n");
    return true;
}

bool
gicv3_init_from_dtb(const struct devicetree *const tree,
                    const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop *const intr_controller_node =
        devicetree_node_get_prop(node, DEVICETREE_PROP_INTR_CONTROLLER);

    if (intr_controller_node == NULL) {
        printk(LOGLEVEL_WARN,
               "gicv3: dtb-node is missing interrupt-controller property\n");
        return false;
    }

    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN, "gicv3: dtb-node is missing reg property\n");
        return false;
    }

    if (array_item_count(reg_prop->list) != 2) {
        printk(LOGLEVEL_WARN,
               "gicv3: reg prop of dtb node is of the incorrect length\n");
        return false;
    }

    struct devicetree_prop_reg_info *const dist_reg_info =
        array_front(reg_prop->list);

    if (dist_reg_info->size < sizeof(struct gicd_v3_registers)) {
        printk(LOGLEVEL_WARN,
               "gicv3: reg property describes dist range smaller than "
               "needed\n");
        return false;
    }

    struct range redist_range = RANGE_EMPTY();
    struct devicetree_prop_reg_info *const redist_reg_info =
        array_at(reg_prop->list, /*index=*/1);

    if (!range_create_and_verify(redist_reg_info->address,
                                 redist_reg_info->size,
                                 &redist_range))
    {
        printk(LOGLEVEL_WARN, "gicv3: 'reg' property's range overflows\n");
        return false;
    }

    const struct devicetree_prop_other *const redist_count_prop =
        devicetree_node_get_other_prop(node,
                                       SV_STATIC("#redistributor-regions"));

    if (redist_count_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "gicv3: '#redistributor-regions' prop is missing\n");
        return false;
    }

    if (!devicetree_prop_other_get_u32(redist_count_prop, &g_redist_count)) {
        printk(LOGLEVEL_WARN,
               "gicv3: '#redistributor-regions' prop is malformed\n");
        return false;
    }

    if (g_redist_count != 1) {
        printk(LOGLEVEL_WARN, "gicv3: '#redistributor-regions' must be one\n");
        return false;
    }

    if (!gicv3_init_from_info(dist_reg_info->address, redist_range)) {
        return false;
    }

    const struct string_view compat_sv = SV_STATIC("arm,gic-v3-its");
    devicetree_node_foreach_child(node, child_node) {
        if (!devicetree_node_has_compat_sv(child_node, compat_sv)) {
            continue;
        }

        gic_its_init_from_dtb(tree, child_node);
        break;
    }

    return true;
}
