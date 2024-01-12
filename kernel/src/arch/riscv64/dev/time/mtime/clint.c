/*
 * kernel/src/arch/riscv64/dev/time/mtime/clint.c
 * © suhas pai
 */

#include "dev/dtb/node.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "time/clock.h"

struct clint_regs {
    volatile const uint32_t msip[4096];
    volatile uint64_t time_compares[4095];
    volatile uint64_t mtime;
};

struct clint_mtime {
    struct clock clock;
    struct mmio_region *mmio;

    volatile struct clint_regs *regs;
    uint64_t frequency;
};

static struct clint_mtime *g_clint_mtime = NULL;

__optimize(3) struct clock *system_clock_get() {
    return &g_clint_mtime->clock;
}

void
clint_init(volatile uint64_t *const base,
           const uint64_t size,
           const uint64_t freq)
{
    struct mmio_region *const mmio =
        vmap_mmio(RANGE_INIT((uint64_t)base, size),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    if (mmio == NULL) {
        printk(LOGLEVEL_WARN, "clint: failed to mmio-map registers\n");
        return;
    }

    struct clint_mtime *const clint = kmalloc(sizeof(*clint));
    if (clint == NULL) {
        vunmap_mmio(mmio);
        printk(LOGLEVEL_WARN, "clint: failed to allocate clint-info struct\n");

        return;
    }

    clint->clock.name = SV_STATIC("clint");
    clint->mmio = mmio;
    clint->regs = mmio->base;
    clint->frequency = freq;

    clint->clock.resolution = CLOCK_RES_NANO;
    clint->clock.one_shot_capable = false;

    clint->clock.read = NULL;
    clint->clock.enable = NULL;
    clint->clock.disable = NULL;
    clint->clock.oneshot = NULL;

    clock_add(&clint->clock);
    g_clint_mtime = clint;
}

static bool
init_from_dtb(const struct devicetree *const tree,
              const struct devicetree_node *const node)
{
    if (g_clint_mtime != NULL) {
        printk(LOGLEVEL_WARN, "clint: multiple mtime clocks found. Ignoring\n");
        return true;
    }

    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "clint: 'reg' property in dtb node is missing\n");
        return false;
    }

    if (array_empty(reg_prop->list)) {
        printk(LOGLEVEL_WARN,
               "clint: dtb node's 'reg' property is empty\n");
        return false;
    }

    const struct devicetree_prop_reg_info *const reg =
        (const struct devicetree_prop_reg_info *)array_front(reg_prop->list);

    if (reg->size < sizeof(struct clint_regs)) {
        printk(LOGLEVEL_WARN,
               "clint: base-addr reg of 'reg' property of dtb node is too "
               "small\n");
        return false;
    }

    struct devicetree_node *const cpus_node =
        devicetree_get_node_at_path(tree, SV_STATIC("/cpus"));

    if (cpus_node == NULL) {
        printk(LOGLEVEL_WARN,
               "clint: failed to init because node at path /cpus is missing\n");
        return false;
    }

    const struct devicetree_prop_other *const timebase_freq_prop =
        devicetree_node_get_other_prop(cpus_node,
                                       SV_STATIC("timebase-frequency"));

    if (timebase_freq_prop == NULL) {
        printk(LOGLEVEL_WARN,
               "clint: node at path \"/cpus\" is missing the "
               "timebase-frequency prop\n");
        return false;
    }

    uint32_t freq = 0;
    if (!devicetree_prop_other_get_u32(timebase_freq_prop, &freq)) {
        printk(LOGLEVEL_WARN,
               "clint: node at path \"/cpus\" timebase-frequency prop is of "
               "the wrong length %" PRIu32 "\n",
               timebase_freq_prop->data_length);
        return false;
    }

    clint_init((volatile uint64_t *)reg->address, reg->size, freq);
    return true;
}

static const struct string_view compat_list[] = {
    SV_STATIC("sifive,clint0"), SV_STATIC("riscv,clint0")
};

static const struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .match_flags = __DTB_DRIVER_MATCH_COMPAT,

    .compat_list = compat_list,
    .compat_count = countof(compat_list),
};

__driver static const struct driver driver = {
    .name = SV_STATIC("riscv64-clint-driver"),
    .dtb = &dtb_driver,
    .pci = NULL
};