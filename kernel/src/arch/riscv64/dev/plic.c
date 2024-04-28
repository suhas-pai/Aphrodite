/*
 * kernel/src/arch/riscv64/dev/plic.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "mm/mmio.h"

#include "plic.h"

enum plic_irq_kind {
    PLIC_IRQ_SOFTWARE_USER,
    PLIC_IRQ_SOFTWARE_SUPERVISOR,
    PLIC_IRQ_SOFTWARE_HYPERVISOR,
    PLIC_IRQ_SOFTWARE_MACHINE,

    PLIC_IRQ_TIMER_USER,
    PLIC_IRQ_TIMER_SUPERVISOR,
    PLIC_IRQ_TIMER_HYPERVISOR,
    PLIC_IRQ_TIMER_MACHINE,

    PLIC_IRQ_EXTERNAL_USER,
    PLIC_IRQ_EXTERNAL_SUPERVISOR,
    PLIC_IRQ_EXTERNAL_HYPERVISOR,
    PLIC_IRQ_EXTERNAL_MACHINE,
};

struct hart_interrupt_enable_control {
    volatile uint32_t enable[128];
};

struct hart_interrupt_context {
    volatile uint32_t context[1024];
};

struct plic_registers {
    char reserved[0x2000];

    struct hart_interrupt_enable_control hart_intr_enable[240];
    struct hart_interrupt_context hart_intr_context[240];
};

static volatile struct plic_registers *g_regs = NULL;
static struct mmio_region *g_mmio = NULL;

__optimize(3)
static inline bool plic_irq_kind_is_valid(const enum plic_irq_kind kind) {
    switch (kind) {
        case PLIC_IRQ_SOFTWARE_USER:
        case PLIC_IRQ_SOFTWARE_SUPERVISOR:
        case PLIC_IRQ_SOFTWARE_HYPERVISOR:
        case PLIC_IRQ_SOFTWARE_MACHINE:
        case PLIC_IRQ_TIMER_USER:
        case PLIC_IRQ_TIMER_SUPERVISOR:
        case PLIC_IRQ_TIMER_HYPERVISOR:
        case PLIC_IRQ_TIMER_MACHINE:
        case PLIC_IRQ_EXTERNAL_USER:
        case PLIC_IRQ_EXTERNAL_SUPERVISOR:
        case PLIC_IRQ_EXTERNAL_HYPERVISOR:
        case PLIC_IRQ_EXTERNAL_MACHINE:
            return true;
    }

    return false;
}

__optimize(3) static inline
const char *plic_irq_kind_get_string(const enum plic_irq_kind kind) {
    switch (kind) {
        case PLIC_IRQ_SOFTWARE_USER:
            return "software-user";
        case PLIC_IRQ_SOFTWARE_SUPERVISOR:
            return "software-supervisor";
        case PLIC_IRQ_SOFTWARE_HYPERVISOR:
            return "software-hypervisor";
        case PLIC_IRQ_SOFTWARE_MACHINE:
            return "software-machine";
        case PLIC_IRQ_TIMER_USER:
            return "timer-user";
        case PLIC_IRQ_TIMER_SUPERVISOR:
            return "timer-supervisor";
        case PLIC_IRQ_TIMER_HYPERVISOR:
            return "timer-hypervisor";
        case PLIC_IRQ_TIMER_MACHINE:
            return "timer-machine";
        case PLIC_IRQ_EXTERNAL_USER:
            return "external-user";
        case PLIC_IRQ_EXTERNAL_SUPERVISOR:
            return "external-supervisor";
        case PLIC_IRQ_EXTERNAL_HYPERVISOR:
            return "external-hypervisor";
        case PLIC_IRQ_EXTERNAL_MACHINE:
            return "external-machine";
    }

    verify_not_reached();
}

bool
plic_init_from_dtb(const struct devicetree *const tree,
                   const struct devicetree_node *const node)
{
    (void)tree;
    const struct devicetree_prop *const intr_cntlr_prop =
        devicetree_node_get_prop(node, DEVICETREE_PROP_INTR_CONTROLLER);

    if (intr_cntlr_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "plic: dtb-node is missing a 'interrupt-controller' property\n");
        return false;
    }

    uint32_t ndev = 0;
    struct range reg_range = RANGE_EMPTY();

    const fdt32_t *intr_ext_list = NULL;
    uint32_t intr_ext_count = 0;

    {
        const struct devicetree_prop_other *const ndev_prop =
            devicetree_node_get_other_prop(node, SV_STATIC("riscv,ndev"));

        if (ndev_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node is missing \"riscv,ndev\" property\n");
            return false;
        }

        if (!devicetree_prop_other_get_u32(ndev_prop, &ndev)) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node's 'riscv,ndev' prop is not an uint32\n");
            return false;
        }
    }
    {
        const struct devicetree_prop_reg *const reg_prop =
            (const struct devicetree_prop_reg *)
                (uint64_t)devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

        if (reg_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node is missing a 'reg' property\n");
            return false;
        }

        if (array_item_count(reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node has a malformed 'reg' property\n");
            return false;
        }

        const struct devicetree_prop_reg_info *const reg_info =
            array_front(reg_prop->list);

        if (reg_info->size < sizeof(struct plic_registers)) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node details memory space too small for "
                   "registers\n");
            return false;
        }

        reg_range = RANGE_INIT(reg_info->address, reg_info->size);
    }
    {
        const struct devicetree_prop_other *const intr_ext_prop =
            devicetree_node_get_other_prop(node,
                                           SV_STATIC("interrupts-extended"));

        if (intr_ext_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node is missing 'interrupts-extended' "
                   "property\n");
            return false;
        }

        if (!devicetree_prop_other_get_u32_list(intr_ext_prop,
                                                /*u32_in_elem_count=*/2,
                                                &intr_ext_list,
                                                &intr_ext_count))
        {
            printk(LOGLEVEL_WARN,
                "plic: dtb-node has a malformed 'interrupts-extended' "
                "property\n");
            return false;
        }
    }

    g_mmio = vmap_mmio(reg_range, PROT_READ | PROT_WRITE, /*flags=*/0);
    if (g_mmio == NULL) {
        printk(LOGLEVEL_WARN, "plic: failed to map registers\n");
        return false;
    }

    g_regs = g_mmio->base;

    const fdt32_t *intr_ext_ptr = intr_ext_list;
    for (uint32_t i = 0; i != intr_ext_count; i++, intr_ext_ptr += 2) {
        const uint32_t hart_id = fdt32_to_cpu(intr_ext_ptr[0]);
        const enum plic_irq_kind kind = fdt32_to_cpu(intr_ext_ptr[1]);

        if (!plic_irq_kind_is_valid(kind)) {
            printk(LOGLEVEL_WARN,
                   "plic: dtb-node's interrupts-extended prop's interrupt "
                   "#%" PRIu32 " has an unknown irq kind\n",
                   i + 1);
            continue;
        }

        printk(LOGLEVEL_INFO,
               "plic: found %s interrupt for hart %" PRIu32 "\n",
               plic_irq_kind_get_string(kind),
               hart_id);
    }

    return true;
}