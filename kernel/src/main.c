/*
 * kernel/src/main.c
 * © suhas pai
 */

#include <limine.h>

#include "asm/irqs.h"
#include "cpu/isr.h"

#include "dev/init.h"
#include "dev/printk.h"

#include "mm/early.h"
#include "mm/page_alloc.h"

#include "sys/boot.h"

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

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

void test_alloc_largepage() {
    struct page *const largepage =
        alloc_large_page(__ALLOC_ZERO, LARGEPAGE_LEVEL_1GIB);

    if (largepage != NULL) {
        printk(LOGLEVEL_INFO,
               "kernel: allocated largepage at %p\n",
               (void *)page_to_phys(largepage));

        free_large_page(largepage);
        printk(LOGLEVEL_INFO, "kernel: freed largepage\n");
    } else {
        printk(LOGLEVEL_WARN, "kernel: failed to allocate a 1gib page\n");
    }
}

void arch_init();
void arch_early_init();

// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.
void _start(void) {
    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1)
    {
        hcf();
    }

    // Fetch the first framebuffer.
    struct limine_framebuffer *const framebuffer =
        framebuffer_request.response->framebuffers[0];

    // Note: we assume the framebuffer model is RGB with 32-bit pixels.
    volatile uint32_t *const fb_ptr = framebuffer->address;
    for (size_t i = 0; i < 100; i++) {
        fb_ptr[i * (framebuffer->pitch / 4) + i] = 0xffffff;
    }

    boot_early_init();
    arch_early_init();

    serial_init();
    printk(LOGLEVEL_INFO, "Console is working?\n");

    boot_init();
    mm_early_init();

    arch_init();
    isr_init();
    dev_init();

    printk(LOGLEVEL_INFO, "kernel: finished initializing\n");
    test_alloc_largepage();

    // We're done, just hang...
    enable_all_interrupts();
    hcf();
}
