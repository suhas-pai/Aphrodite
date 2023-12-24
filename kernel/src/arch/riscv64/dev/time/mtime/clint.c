/*
 * kernel/src/arch/riscv64/dev/time/mtime/clint.c
 * © suhas pai
 */

#include "dev/driver.h"

#include "lib/util.h"
#include "mm/kmalloc.h"
#include "sys/mmio.h"

#include "time/clock.h"
#include "time/kstrftime.h"

#define CLINT_REG_MTIME 0xBFF8

struct clint_mtime {
    struct clock clock;
    struct mmio_region *mmio;

    volatile void *base;
    uint64_t frequency;
};

__optimize(3)
__unused static uint64_t clint_mtime_read(const struct clock *const source) {
    struct clint_mtime *const clint =
        container_of(source, struct clint_mtime, clock);
    const msec_t seconds =
        mmio_read(reg_to_ptr(uint32_t, clint->base, CLINT_REG_MTIME));

    return seconds / clint->frequency;
}

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
    clint->base = mmio->base;
    clint->frequency = freq;
    clint->clock.resolution = CLOCK_RES_NANO;

    clint->clock.read = clint_mtime_read;
    clint->clock.enable = NULL;
    clint->clock.disable = NULL;
    clint->clock.resume = NULL;
    clint->clock.suspend = NULL;

    clock_add(&clint->clock);
    g_clint_mtime = clint;

    const usec_t stamp = clint_mtime_read(&clint->clock);
    printk(LOGLEVEL_INFO, "clint: timestamp is %" PRIu64 "\n", stamp);

    const struct tm tm = tm_from_stamp(stamp);
    printk_strftime(LOGLEVEL_INFO, "clint: date is %c\n", &tm);
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
               "clint: 'reg' property in dtb is missing\n");
        return false;
    }

    if (array_empty(reg_prop->list)) {
        printk(LOGLEVEL_WARN, "clint: reg property is missing\n");
        return false;
    }

    const struct devicetree_prop_reg_info *const reg =
        (const struct devicetree_prop_reg_info *)array_front(reg_prop->list);

    if (!index_range_in_bounds(RANGE_INIT(CLINT_REG_MTIME, sizeof(uint64_t)),
                               reg->size))
    {
        printk(LOGLEVEL_WARN,
               "clint: base-addr reg of 'reg' property of dtb node is "
               "too small\n");
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

    if (timebase_freq_prop->data_length != sizeof(fdt32_t)) {
        printk(LOGLEVEL_WARN,
               "clint: node at path \"/cpus\" timebase-frequency prop is of "
               "the wrong length %" PRIu64 "\n",
               sizeof(fdt32_t));
        return false;
    }

    clint_init((volatile uint64_t *)reg->address,
               reg->size,
               /*freq=*/fdt32_to_cpu(*timebase_freq_prop->data));

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
    .name = "riscv64-clint-driver",
    .dtb = &dtb_driver,
    .pci = NULL
};