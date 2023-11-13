/*
 * kernel/src/main.c
 * © suhas pai
 */

#include <limine.h>

#include "asm/irqs.h"
#include "cpu/isr.h"

#include "dev/flanterm.h"
#include "dev/init.h"
#include "dev/printk.h"

#include "mm/early.h"
#include "mm/page_alloc.h"

#include "sys/boot.h"

// Set the base revision to 1, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

LIMINE_BASE_REVISION(1)

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#endif
    }
}

static void test_alloc_largepage() {
    struct page *const largepage =
#if defined(__aarch64__) && defined(AARCH64_USE_16K_PAGES)
        alloc_large_page(__ALLOC_ZERO, LARGEPAGE_LEVEL_32MIB);
#else
        alloc_large_page(__ALLOC_ZERO, LARGEPAGE_LEVEL_1GIB);
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
        hcf();
    }

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    boot_init();
    setup_flanterm();

    mm_early_init();
    arch_early_init();

    serial_init();
    printk(LOGLEVEL_INFO, "Console is working?\n");

    boot_post_early_init();
    mm_init();

    arch_init();
    arch_post_mm_init();

    isr_init();
    dev_init();

    printk(LOGLEVEL_INFO, "kernel: finished initializing\n");
    test_alloc_largepage();

    // We're done, just hang...
    enable_all_interrupts();
    hcf();
}
