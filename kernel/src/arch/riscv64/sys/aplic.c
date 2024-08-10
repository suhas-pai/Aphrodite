/*
 * kernel/src/arch/riscv64/sys/aplic.c
 * © suhas pai
 */

#include "asm/privl.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "sched/thread.h"
#include "sys/mmio.h"

#include "imsic.h"

enum aplic_domain_config_flags {
    __APLIC_DOMAIN_CFG_BIG_ENDIAN = 1 << 0,
    __APLIC_DOMAIN_CFG_DIRECT_MODE = 1 << 2,
    __APLIC_DOMAIN_CFG_ENABLE = 1 << 8
};

enum aplic_irq_source_mode {
    APLIC_IRQ_SOURCE_MODE_INACTIVE,
    APLIC_IRQ_SOURCE_MODE_DETACHED,

    APLIC_IRQ_SOURCE_MODE_RISING_EDGE = 4,
    APLIC_IRQ_SOURCE_MODE_FALLING_EDGE,

    APLIC_IRQ_SOURCE_MODE_LEVEL_HIGH,
    APLIC_IRQ_SOURCE_MODE_LEVEL_LOW,
};

enum aplic_generate_msi_shifts {
    APLIC_GENERATE_MSI_HART_SHIFT = 18,
};

enum aplic_target_shifts {
    APLIC_TARGET_GUEST_SHIFT = 12,
    APLIC_TARGET_HART_SHIFT = 18,
};

struct aplic_registers {
    uint32_t domain_config;
    uint32_t source_config[1023];

    const char reserved[3008];

    uint32_t machine_msi_config_addr_low32;
    uint32_t machine_msi_config_addr_high32;

    uint32_t supervisor_msi_config_addr_low32;
    uint32_t supervisor_msi_config_addr_high32;

    const char reserved_2[48];

    uint32_t set_pending[55];
    uint32_t set_irq_pending;

    const char reserved_3[32];

    uint32_t clear_pending[55];
    uint32_t clear_irq_pending;

    const char reserved_4[32];

    uint32_t set_enable[55];
    uint32_t set_irq_enable;

    const char reserved_5[32];

    uint32_t clear_enable[55];
    uint32_t clear_irq_enable;

    const char reserved_6[32];

    uint32_t set_irq_pending_le;
    uint32_t set_irq_pending_be;

    const char reserved_7[4088];

    uint32_t generate_msi;
    uint32_t target[1023];

    // Unused because we have an imsic
    uint32_t idc;
};

struct aplic {
    struct mmio_region *mmio;
    volatile struct aplic_registers *regs;

    uint32_t gsi_base;
    uint32_t source_count;
    uint32_t phandle;
};

#define APLIC_IRQ_COUNT 1024

static struct aplic *g_machine_aplic = NULL;
static struct aplic *g_supervisor_aplic = NULL;

static int64_t g_machine_child_phandle = -1;

static void
init_with_regs(volatile struct aplic_registers *const regs,
               const uint16_t source_count)
{
    preempt_disable();
    volatile void *const imsic_region =
        imsic_region_for_hartid(this_cpu()->hart_id);

    preempt_enable();

    // DIrect mode must be supported by aplic
    assert(mmio_read(&regs->domain_config) & __APLIC_DOMAIN_CFG_DIRECT_MODE);

    mmio_write(&regs->domain_config,
               __APLIC_DOMAIN_CFG_ENABLE | __APLIC_DOMAIN_CFG_DIRECT_MODE);
    mmio_write(&regs->supervisor_msi_config_addr_low32,
               (uint64_t)imsic_region >> 12);
    mmio_write(&regs->supervisor_msi_config_addr_high32,
               (uint64_t)imsic_region >> (32 + 12));

    for (uint16_t irq = 0; irq < source_count; irq++) {
        mmio_write(&regs->source_config[irq + 1],
                   APLIC_IRQ_SOURCE_MODE_RISING_EDGE);
    }
}

bool
aplic_init(struct aplic *const aplic,
           const struct range range,
           const uint16_t source_count,
           const uint32_t gsi_base)
{
    struct mmio_region *const mmio =
        vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN, "aplic: failed to mmio-map reg range\n");
        return NULL;
    }

    printk(LOGLEVEL_INFO, "aplic: supports %" PRIu32 " irqs\n", source_count);

    aplic->mmio = mmio;
    aplic->regs = mmio->base;
    aplic->gsi_base = gsi_base;
    aplic->source_count = source_count;

    init_with_regs(aplic->regs, source_count);
    return aplic;
}

__debug_optimize(3) struct aplic *aplic_for_privl(const enum riscv64_privl privl) {
    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            return g_machine_aplic;
        case RISCV64_PRIVL_SUPERVISOR:
            return g_supervisor_aplic;
    }

    verify_not_reached();
}

__debug_optimize(3)
bool aplic_enable_irq(const enum riscv64_privl privl, const uint16_t irq) {
    struct aplic *const aplic = aplic_for_privl(privl);
    const uint32_t irq_end = aplic->gsi_base + aplic->source_count;

    if (irq >= aplic->gsi_base && irq < irq_end) {
        mmio_write(&aplic->regs->set_irq_enable, irq);
        return true;
    }

    return false;
}

__debug_optimize(3)
bool aplic_disable_irq(const enum riscv64_privl privl, const uint16_t irq) {
    struct aplic *const aplic = aplic_for_privl(privl);
    const uint32_t irq_end = aplic->gsi_base + aplic->source_count;

    if (irq >= aplic->gsi_base && irq < irq_end) {
        mmio_write(&aplic->regs->clear_irq_enable, irq);
        return true;
    }

    return false;
}

bool
aplic_set_target_msi(const enum riscv64_privl privl,
                     const uint32_t irq,
                     const uint32_t hart_id,
                     const uint32_t guest,
                     const uint32_t eiid)
{
    struct aplic *const aplic = aplic_for_privl(privl);
    const uint32_t irq_end = aplic->gsi_base + aplic->source_count;

    if (irq < aplic->gsi_base && irq >= irq_end) {
        return false;
    }

    mmio_write(&aplic->regs->target[irq],
               hart_id << APLIC_TARGET_HART_SHIFT
               | guest << APLIC_TARGET_GUEST_SHIFT
               | eiid);

    return true;
}

__debug_optimize(3) void
aplic_set_msi_address(const enum riscv64_privl privl, const uint64_t address) {
    struct aplic *const aplic = aplic_for_privl(privl);

    mmio_write(&aplic->regs->supervisor_msi_config_addr_low32, address >> 12);
    mmio_write(&aplic->regs->supervisor_msi_config_addr_high32, address >> 44);
}

bool
aplic_init_from_acpi(const struct range range,
                     const uint16_t source_count,
                     const uint32_t gsi_base)
{
    g_supervisor_aplic = kmalloc(sizeof(struct aplic));
    if (g_supervisor_aplic == NULL) {
        return false;
    }

    return aplic_init(g_supervisor_aplic, range, source_count, gsi_base);
}

bool
aplic_init_from_dtb(const struct devicetree *const tree,
                    const struct devicetree_node *const node)
{
    (void)tree;
    {
        const struct devicetree_prop *const interrupt_controller =
            devicetree_node_get_prop(node, DEVICETREE_PROP_INTR_CONTROLLER);

        if (interrupt_controller == NULL) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'interrupt-controller' "
                   "prop\n");

            return false;
        }
    }

    struct aplic **aplic_ptr = NULL;
    struct range reg_range = RANGE_EMPTY();

    uint32_t source_count = 0;
    uint32_t phandle = 0;

    {
        const struct devicetree_prop_phandle *const phandle_prop =
            (const struct devicetree_prop_phandle *)
                devicetree_node_get_prop(node, DEVICETREE_PROP_PHANDLE);

        if (phandle_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node's 'phandle' prop is missing\n");
            return false;
        }

        phandle = phandle_prop->phandle;
    }
    {
        const struct devicetree_prop_other *const children_prop =
            devicetree_node_get_other_prop(node, SV_STATIC("riscv,children"));

        if (children_prop != NULL) {
            const fdt32_t *child_list = NULL;
            uint32_t count = 0;

            if (!devicetree_prop_other_get_u32_list(children_prop,
                                                    /*u32_in_elem_count=*/1,
                                                    &child_list,
                                                    &count))
            {
                printk(LOGLEVEL_WARN,
                       "aplic: dtb-node's 'riscv,children' prop is "
                       "malformed\n");
                return false;
            }

            if (count != 1) {
                printk(LOGLEVEL_WARN,
                       "aplic: 'riscv,children' prop has multiple children\n");
                return false;
            }

            g_machine_child_phandle = fdt32_to_cpu(child_list[0]);
            if (g_supervisor_aplic != NULL) {
                if (g_supervisor_aplic->phandle != g_machine_child_phandle) {
                    printk(LOGLEVEL_WARN,
                           "aplic: 'riscv,children' doesn't point to "
                           "supervisor aplic\n");
                    return false;
                }
            }

            aplic_ptr = &g_machine_aplic;
        } else {
            if (g_machine_child_phandle != -1) {
                if (g_machine_child_phandle != phandle) {
                    printk(LOGLEVEL_WARN,
                           "aplic: supervisor's phandle isn't pointed to by "
                           "machine aplic\n");
                    return false;
                }
            }

            aplic_ptr = &g_supervisor_aplic;
        }

        *aplic_ptr = kmalloc(sizeof(struct aplic));
        if (*aplic_ptr == NULL) {
            printk(LOGLEVEL_WARN, "aplic: failed to alloc info\n");
            return false;
        }

        (*aplic_ptr)->phandle = phandle;
    }
    {
        const struct devicetree_prop_reg *const reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

        if (reg_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'reg' property\n");
            return false;
        }

        if (array_item_count(reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node's 'reg' property is of the incorrect "
                   "length\n");
            return false;
        }

        struct devicetree_prop_reg_info *const reg_info =
            array_front(reg_prop->list);

        if (reg_info->size < sizeof(struct aplic_registers)) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node's 'reg' property is too small\n");
            return false;
        }

        if (!range_create_and_verify(reg_info->address,
                                     reg_info->size,
                                     &reg_range))
        {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node's 'reg' property's range overflows\n");
            return false;
        }
    }
    {
        const struct devicetree_prop_other *const source_prop =
            devicetree_node_get_other_prop(node,
                                           SV_STATIC("riscv,num-sources"));

        if (source_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'riscv,num-sources' "
                   "property\n");
            return false;
        }

        if (!devicetree_prop_other_get_u32(source_prop, &source_count)) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node is missing a 'riscv,num-sources' "
                   "property\n");
            return false;
        }

        if (source_count == 0) {
            printk(LOGLEVEL_WARN,
                   "aplic: dtb-node specifies zero sources (irqs) in its "
                   "'riscv,num-sources' property\n");
            return false;
        }
    }

    return aplic_init(*aplic_ptr, reg_range, source_count, /*gsi_base=*/0);
}

static const struct string_view compat[] = { SV_STATIC("ricsv,aplic") };
static const struct dtb_driver dtb_driver = {
    .init = aplic_init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat,
    .compat_count = countof(compat),
};

__driver static const struct driver driver = {
    .name = SV_STATIC("riscv,aplic"),
    .dtb = &dtb_driver,
    .pci = NULL
};
