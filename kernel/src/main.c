/*
 * kernel/src/main.c
 * © suhas pai
 */

#include "dev/dtb/init.h"

#include "asm/irqs.h"
#include "acpi/api.h"

#include "cpu/isr.h"
#include "cpu/util.h"

#include "dev/flanterm.h"
#include "dev/init.h"
#include "dev/printk.h"

#include "mm/early.h"
#include "mm/page_alloc.h"

#if defined(__x86_64__) || defined(__riscv64)
    #include "sched/scheduler.h"
#endif /* defined(__x86_64__) */

#include "sys/boot.h"

// Set the base revision to 1, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

LIMINE_BASE_REVISION(1)

static void test_alloc_largepage() {
    struct page *const largepage =
#if defined(__aarch64__) && defined(AARCH64_USE_16K_PAGES)
        alloc_large_page(LARGEPAGE_LEVEL_32MIB, __ALLOC_ZERO);
#else
        alloc_large_page(LARGEPAGE_LEVEL_1GIB, __ALLOC_ZERO);
#endif /* defined(__aarch64__) && defined(AARCH64_USE_16K_PAGES) */

    if (largepage != NULL) {
        printk(LOGLEVEL_INFO,
               "kernel: allocated largepage at %p\n",
               (void *)page_to_phys(largepage));

        free_large_page(largepage);
        printk(LOGLEVEL_INFO, "kernel: freed largepage\n");
    } else {
#if defined(__aarch64__) && defined(AARCH64_USE_16K_PAGES)
        printk(LOGLEVEL_WARN, "kernel: failed to allocate a 32mib page\n");
#else
        printk(LOGLEVEL_WARN, "kernel: failed to allocate a 1gib page\n");
#endif /* defined(__aarch64__) && defined(AARCH64_USE_16K_PAGES) */
    }
}

void arch_init();
void arch_early_init();
void arch_post_mm_init();

// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.
void _start(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        cpu_idle();
    }

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    boot_init();
    acpi_parse_tables();
    setup_flanterm();

    mm_early_init();
    arch_early_init();

    serial_init();
    printk(LOGLEVEL_INFO, "Console is working?\n");

    boot_post_early_init();
    mm_init();

    arch_init();
    arch_post_mm_init();

    dtb_parse_main_tree();

    isr_init();
    dev_init();

    test_alloc_largepage();

    // We're done, just hang...
    enable_interrupts();

#if defined(__x86_64__) || defined(__riscv64)
    sched_init();
    printk(LOGLEVEL_INFO, "kernel: finished initializing\n");

    sched_yield(/*noreturn=*/true);
#else
    printk(LOGLEVEL_INFO, "kernel: finished initializing\n");
    cpu_idle();
#endif
}
