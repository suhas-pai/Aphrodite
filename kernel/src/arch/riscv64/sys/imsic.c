/*
 * kernel/src/arch/riscv64/sys/imsic.c
 * © suhas pai
 */

#include "lib/adt/bitset.h"

#include "asm/irqs.h"
#include "cpu/spinlock.h"
#include "dev/printk.h"
#include "lib/util.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/aplic.h"
#include "imsic.h"

enum {
    EP_BASE = 0x80,
    EI_BASE = 0xc0,

    EI_DELIVERY = 0x70,
    EI_THRESHOLD = 0x72,
};

#define IMSIC_MSG_COUNT 64

struct imsic_region {
    uint64_t hart_id;
    uint64_t address;

    struct mmio_region *mmio;
};

struct imsic {
    uint8_t guest_index_bits;
    uint32_t interrupt_id_count;

    struct spinlock lock;
    uint64_t bitset;

    isr_func_t funcs[IMSIC_MSG_COUNT];
};

static struct imsic *g_machine_imsic = NULL;
static struct imsic *g_supervisor_imsic = NULL;

static struct array g_supervisor_region_list =
    ARRAY_INIT(sizeof(struct imsic_region));

__debug_optimize(3) struct imsic *imsic_for_privl(const enum riscv64_privl privl) {
    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            return g_machine_imsic;
        case RISCV64_PRIVL_SUPERVISOR:
            return g_supervisor_imsic;
    }

    verify_not_reached();
}

static void
imsic_init(const enum riscv64_privl privl,
           const uint8_t guest_index_bits,
           const uint32_t intr_id_count)
{
    struct imsic *const imsic = imsic_for_privl(privl);

    imsic->guest_index_bits = guest_index_bits;
    imsic->interrupt_id_count = intr_id_count;
    imsic->bitset = 1;
}

void
imsic_init_from_acpi(const enum riscv64_privl privl,
                     const uint8_t guest_index_bits,
                     const uint32_t intr_id_count)
{
    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            g_machine_imsic = kmalloc(sizeof(struct imsic));
            assert_msg(g_machine_imsic != NULL,
                       "imsic_init_from_acpi(): failed to alloc machine imsic");

            break;
        case RISCV64_PRIVL_SUPERVISOR:
            g_supervisor_imsic = kmalloc(sizeof(struct imsic));
            assert_msg(g_supervisor_imsic != NULL,
                       "imsic_init_from_acpi(): failed to alloc supervisor "
                       "imsic");

            break;
    }

    imsic_init(privl, guest_index_bits, intr_id_count);
}

__debug_optimize(3) static enum riscv64_privl
find_privl_for_imsic(struct imsic *const imsic, const uint32_t phandle) {
    struct aplic *aplic = aplic_for_privl(RISCV64_PRIVL_MACHINE);
    if (aplic->msi_parent == phandle) {
        aplic->imsic = imsic;
        return RISCV64_PRIVL_MACHINE;
    }

    aplic = aplic_for_privl(RISCV64_PRIVL_SUPERVISOR);
    if (aplic->msi_parent == phandle) {
        aplic->imsic = imsic;
        return RISCV64_PRIVL_SUPERVISOR;
    }

    panic("failed to find aplic for imsic with phandle %" PRIu32 "\n", phandle);
}

bool
imsic_init_from_dtb(const struct devicetree *const tree,
                    const struct devicetree_node *const node)
{
    (void)tree;
    {
        const struct devicetree_prop *const intr_controller =
            devicetree_node_get_prop(node, DEVICETREE_PROP_INTR_CONTROLLER);

        if (intr_controller == NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node is missing an 'interrupt-controller' "
                   "prop\n");

            return false;
        }
    }
    {
        const struct devicetree_prop *const msi_controller =
            devicetree_node_get_prop(node, DEVICETREE_PROP_MSI_CONTROLLER);

        if (msi_controller == NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node is missing a 'msi-controller' prop\n");

            return false;
        }
    }

    struct range reg_range = RANGE_EMPTY();

    uint8_t guest_index_bits = 0;
    uint16_t intr_id_count = 0;
    uint32_t phandle = 0;

    {
        const struct devicetree_prop_phandle *const phandle_prop =
            (const struct devicetree_prop_phandle *)
                devicetree_node_get_prop(node, DEVICETREE_PROP_PHANDLE);

        if (phandle_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'phandle' prop is missing\n");
            return false;
        }

        phandle = phandle_prop->phandle;
    }
    {
        const struct devicetree_prop_phandle *const phandle_prop =
            (const struct devicetree_prop_phandle *)
                devicetree_node_get_prop(node, DEVICETREE_PROP_PHANDLE);

        if (phandle_prop == NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'phandle' prop is missing\n");
            return false;
        }

        phandle = phandle_prop->phandle;
    }
    {
        const struct devicetree_prop_reg *const reg_prop =
            (const struct devicetree_prop_reg *)(uint64_t)
                devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

        if (reg_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node is missing a 'reg' property\n");
            return false;
        }

        if (array_item_count(reg_prop->list) != 1) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'reg' property is of the incorrect "
                   "length\n");
            return false;
        }

        struct devicetree_prop_reg_info *const reg =
            array_front(reg_prop->list);

        if (reg->size < PAGE_SIZE) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'reg' property is too small\n");
            return false;
        }

        if (!range_create_and_verify(reg->address, reg->size, &reg_range)) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'reg' property's range overflows\n");
            return false;
        }
    }
    {
        const struct devicetree_prop_other *const source_prop =
            devicetree_node_get_other_prop(node,
                                           SV_STATIC("riscv,guest-index-bits"));

        if (source_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node is missing a 'riscv,guest-index-bits' "
                   "property\n");
            return false;
        }

        uint32_t value = 0;
        if (!devicetree_prop_other_get_u32(source_prop, &value)) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'riscv,guest-index-bits' property is "
                   "malformed\n");
            return false;
        }

        if (value >= sizeof_bits(uint64_t)) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'riscv,guest-index-bits' property is "
                   "too large\n");
            return false;
        }
    }
    {
        const struct devicetree_prop_other *const source_prop =
            devicetree_node_get_other_prop(node, SV_STATIC("riscv,num-ids"));

        if (source_prop != NULL) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node is missing a 'riscv,num-ids' property\n");
            return false;
        }

        uint32_t value = 0;
        if (!devicetree_prop_other_get_u32(source_prop, &value)) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'riscv,num-ids' property is malformed\n");
            return false;
        }

        if (value == 0) {
            printk(LOGLEVEL_WARN,
                   "imsic: dtb-node's 'riscv,num-ids' property is zero\n");
            return false;
        }
    }

    struct imsic *const imsic = kmalloc(sizeof(*imsic));
    assert_msg(imsic != NULL, "imsic: failed to allocate info\n");

    const enum riscv64_privl privl = find_privl_for_imsic(imsic, phandle);

    imsic_init(privl, guest_index_bits, intr_id_count);
    imsic_enable(privl);

    return true;
}

void imsic_enable(const enum riscv64_privl privl) {
    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            csr_write(miselect, EI_DELIVERY);
            csr_write(mireg, 1);

            csr_write(miselect, EI_THRESHOLD);
            csr_write(mireg, 11);

            return;
        case RISCV64_PRIVL_SUPERVISOR:
            csr_write(siselect, EI_DELIVERY);
            csr_write(sireg, 1);

            csr_write(siselect, EI_THRESHOLD);
            csr_write(sireg, 11);

            return;
    }

    verify_not_reached();
}

__debug_optimize(3) volatile void *
imsic_add_region(const uint64_t hart_id, const struct range range) {
    struct mmio_region *const mmio =
        vmap_mmio(range, PROT_READ | PROT_WRITE, /*flags=*/0);

    assert_msg(mmio != NULL,
               "imsic: failed to map mmio region at " RANGE_FMT,
               RANGE_FMT_ARGS(range));

    const struct imsic_region region = {
        .mmio = mmio,
        .address = range.front,
        .hart_id = hart_id
    };

    assert(array_append(&g_supervisor_region_list, &region));
    return mmio->base;
}

__debug_optimize(3)
volatile void *imsic_region_for_hartid(const uint64_t hart_id) {
    array_foreach(&g_supervisor_region_list, struct imsic_region, region) {
        if (region->hart_id == hart_id) {
            return region->mmio->base;
        }
    }

    return NULL;
}

__debug_optimize(3) uint8_t imsic_alloc_msg(const enum riscv64_privl privl) {
    struct imsic *const imsic = imsic_for_privl(privl);

    const int flag = spin_acquire_save_irq(&imsic->lock);
    const uint64_t msg =
        bitset_find_unset(&imsic->bitset,
                          /*length=*/sizeof_bits(imsic->bitset),
                          /*invert=*/true);

    spin_release_restore_irq(&imsic->lock, flag);
    if (msg == BITSET_INVALID) {
        return UINT8_MAX;
    }

    imsic_enable_msg(privl, msg);
    return msg;
}

__debug_optimize(3)
void imsic_free_msg(const enum riscv64_privl privl, const uint8_t msg) {
    assert(index_in_bounds(msg, IMSIC_MSG_COUNT));

    struct imsic *const imsic = imsic_for_privl(privl);
    const int flag = spin_acquire_save_irq(&imsic->lock);

    imsic_disable_msg(privl, msg);
    bitset_unset(&imsic->bitset, msg);

    imsic->funcs[msg] = NULL;
    spin_release_restore_irq(&imsic->lock, flag);
}

__debug_optimize(3) void
imsic_set_msg_handler(const enum riscv64_privl privl,
                      const uint8_t msg,
                      const isr_func_t func)
{
    struct imsic *const imsic = imsic_for_privl(privl);
    const int flag = spin_acquire_save_irq(&imsic->lock);

    imsic->funcs[msg] = func;
    spin_release_restore_irq(&imsic->lock, flag);
}

__debug_optimize(3)
void imsic_enable_msg(const enum riscv64_privl privl, const uint16_t message) {
    const uint16_t bit_offset = message % sizeof_bits(uint32_t);
    const uint64_t offset = EI_BASE + (message / sizeof_bits(uint32_t));

    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            csr_write(miselect, offset);
            csr_write(mireg, csr_read(mireg) | 1ull << bit_offset);

            csr_set(mie, __INTR_MACHINE_EXTERNAL);
            return;
        case RISCV64_PRIVL_SUPERVISOR:
            csr_write(siselect, offset);
            csr_write(sireg, csr_read(sireg) | 1ull << bit_offset);

            csr_set(sie, __INTR_SUPERVISOR_EXTERNAL);
            return;
    }

    verify_not_reached();
}

__debug_optimize(3)
void imsic_disable_msg(const enum riscv64_privl privl, const uint16_t message) {
    const uint16_t bit_offset = message % sizeof_bits(uint32_t);
    const uint64_t offset = EI_BASE + (message / sizeof_bits(uint32_t));

    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            csr_write(miselect, offset);
            csr_write(mireg, rm_mask(csr_read(mireg), 1ull << bit_offset));

            return;
        case RISCV64_PRIVL_SUPERVISOR:
            csr_write(siselect, offset);
            csr_write(sireg, rm_mask(csr_read(sireg), 1ull << bit_offset));

            return;
    }

    verify_not_reached();
}

__debug_optimize(3) uint32_t imsic_pop(const enum riscv64_privl privl) {
    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            return csr_read_and_zero(mtopei) >> 16;
        case RISCV64_PRIVL_SUPERVISOR:
            return csr_read_and_zero(stopei) >> 16;
    }

    verify_not_reached();
}

__debug_optimize(3) void
imsic_handle(const enum riscv64_privl privl,
             struct thread_context *const context)
{
    const uint32_t code = imsic_pop(privl);
    if (__builtin_expect(!index_in_bounds(code, IMSIC_MSG_COUNT), 0)) {
        printk(LOGLEVEL_WARN,
               "imsic: got interrupt %" PRIu32 ", out-of-bounds\n",
               code);
        return;
    }

    struct imsic *const imsic = imsic_for_privl(privl);
    if (imsic->funcs[code] == NULL) {
        printk(LOGLEVEL_WARN,
               "imsic: got interrupt %" PRIu32 " w/o a handler\n",
               code);
        return;
    }

    imsic->funcs[code](code, context);
    return;
}

__debug_optimize(3)
void imsic_trigger(const enum riscv64_privl privl, const uint16_t message) {
    const uint16_t bit_offset = message % sizeof_bits(uint64_t);
    const uint64_t offset = EP_BASE + 2 * (message / sizeof_bits(uint64_t));

    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            csr_write(siselect, offset);
            csr_write(sireg, csr_read(sireg) | 1ull << bit_offset);

            return;
        case RISCV64_PRIVL_SUPERVISOR:
            csr_write(siselect, offset);
            csr_write(sireg, csr_read(sireg) | 1ull << bit_offset);

            return;
    }

    verify_not_reached();
}

__debug_optimize(3)
void imsic_clear(const enum riscv64_privl privl, const uint16_t message) {
    const uint16_t bit_offset = message % sizeof_bits(uint64_t);
    const uint64_t offset = EP_BASE + 2 * (message / sizeof_bits(uint64_t));

    switch (privl) {
        case RISCV64_PRIVL_MACHINE:
            csr_write(miselect, offset);
            csr_write(mireg, rm_mask(csr_read(mireg), 1ull << bit_offset));

            return;
        case RISCV64_PRIVL_SUPERVISOR:
            csr_write(siselect, offset);
            csr_write(sireg, rm_mask(csr_read(sireg), 1ull << bit_offset));

            return;
    }

    verify_not_reached();
}