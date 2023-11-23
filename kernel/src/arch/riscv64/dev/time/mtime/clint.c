/*
 * kernel/src/arch/riscv64/dev/time/mtime/clint.c
 * © suhas pai
 */

#include "dev/dtb/dtb.h"
#include "dev/driver.h"

#include "dev/printk.h"
#include "fdt/libfdt.h"
#include "lib/util.h"

#include "mm/mmio.h"
#include "sys/mmio.h"

#include "time/time.h"

#define CLINT_REG_MTIME 0xBFF8

struct clint_mtime {
    struct clock_source source;
    struct mmio_region *mmio;

    volatile void *base;
    uint64_t frequency;
};

__optimize(3) __unused
static uint64_t clint_mtime_read(struct clock_source *const source) {
    struct clint_mtime *const clint =
        container_of(source, struct clint_mtime, source);
    const msec_t seconds =
        mmio_read(reg_to_ptr(uint32_t, clint->base, CLINT_REG_MTIME));

    return seconds / clint->frequency;
}

void
clint_init(volatile uint64_t *const base,
           const uint64_t size,
           const uint64_t freq)
{
    (void)base;
    (void)size;
    (void)freq;
#if 0
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

    clint->clock_source.name = "clint";
    clint->mmio = mmio;
    clint->base = mmio->base;
    clint->frequency = freq;

    clint->source.read = clint_mtime_read;
    clint->source.enable = NULL;
    clint->source.disable = NULL;
    clint->source.resume = NULL;
    clint->source.suspend = NULL;

    add_clock_source(&clint->source);

    const usec_t stamp = clint_mtime_read(&clint->source);
    printk(LOGLEVEL_INFO, "clint: timestamp is %" PRIu64 "\n", stamp);

    const struct tm tm = tm_from_stamp(stamp);
    printk_strftime(LOGLEVEL_INFO, "clint: date is %c\n", &tm);
#endif
}

static bool init_from_dtb(const void *const dtb, const int nodeoff) {
    struct dtb_addr_size_pair base_addr_reg = DTB_ADDR_SIZE_PAIR_INIT();
    uint32_t pair_count = 1;

    const bool get_base_addr_reg_result =
        dtb_get_reg_pairs(dtb,
                          nodeoff,
                          /*start_index=*/0,
                          &pair_count,
                          &base_addr_reg);

    if (!get_base_addr_reg_result) {
        printk(LOGLEVEL_WARN,
               "clint: base-addr reg of 'reg' property of dtb node is "
               "malformed\n");
        return false;
    }

    if (!index_range_in_bounds(RANGE_INIT(CLINT_REG_MTIME, sizeof(uint64_t)),
                               base_addr_reg.size))
    {
        printk(LOGLEVEL_WARN,
               "clint: base-addr reg of 'reg' property of dtb node is "
               "too small\n");
        return false;
    }

    const int cpus_offset = fdt_path_offset(dtb, "/cpus");
    if (cpus_offset == -1) {
        printk(LOGLEVEL_WARN, "clint: couldn't find node at path \"/cpus\"\n");
        return false;
    }

    const fdt32_t *data = NULL;
    uint32_t freq_count = 0;

    const bool get_freq_result =
        dtb_get_array_prop(dtb,
                           cpus_offset,
                           "timebase-frequency",
                           &data,
                           &freq_count);

    if (!get_freq_result) {
        printk(LOGLEVEL_WARN, "clint: couldn't get frequency property\n");
        return false;
    }

    clint_init((volatile uint64_t *)base_addr_reg.address,
               base_addr_reg.size,
               /*freq=*/fdt32_to_cpu(*data));

    return true;
}

static const char *compat_list[] = { "sifive,clint0\0riscv,clint0" };
static const struct dtb_driver dtb_driver = {
    .compat_list = compat_list,
    .compat_count = countof(compat_list),
    .init = init_from_dtb
};

__driver static const struct driver driver = {
    .dtb = &dtb_driver,
    .pci = NULL
};