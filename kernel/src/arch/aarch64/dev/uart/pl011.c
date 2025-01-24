/*
 * kernel/src/arch/aarch64/dev/uart/pl011.c
 * © suhas pai
 */

#include "dev/uart/pl011.h"

#include "dev/dtb/bus.h"
#include "dev/dtb/device.h"
#include "dev/dtb/driver.h"

#include "dev/init.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "mm/mmio.h"

#include "sys/mmio.h"

struct pl011_registers {
    volatile uint32_t dr_offset;

    volatile uint32_t padding[5];
    volatile uint32_t fr_offset;

    volatile uint64_t padding_2;
    volatile uint32_t ibrd_offset;
    volatile uint32_t fbrd_offset;
    volatile uint32_t lcr_offset;
    volatile uint32_t cr_offset;

    volatile uint32_t padding_3;
    volatile uint32_t imsc_offset;

    volatile uint32_t padding_4[3];
    volatile uint32_t dmacr_offset;
} __packed;

#define __FR_BUSY (uint32_t)(1 << 3)

#define __CR_TXEN (uint32_t)(1 << 8)
#define __CR_UARTEN (uint32_t)(1 << 0)

#define __LCR_FEN (uint32_t)(1 << 4)
#define __LCR_STP2 (uint32_t)(1 << 3)

#define MAX_ATTEMPTS 10

struct pl011_device {
    struct device device;

    struct terminal term;
    struct spinlock lock;

    volatile struct pl011_registers *regs;
};

// for use when initializing serial before mm/kmalloc
static struct pl011_device early_infos[8] = {0};
static uint8_t early_info_count = 0;

__debug_optimize(3) static
void wait_for_tx_complete(volatile const struct pl011_registers *const dev) {
    for (uint64_t i = 0; i != MAX_ATTEMPTS; i++) {
        if ((mmio_read(&dev->fr_offset) & __FR_BUSY) == 0) {
            return;
        }
    }
}

__debug_optimize(3) static void
pl011_send_char(struct terminal *const term,
                const char ch,
                const uint32_t amount)
{
    struct pl011_device *const info =
        parent_of(term, struct pl011_device, term);

    volatile struct pl011_registers *const device = info->regs;
    wait_for_tx_complete(device);

    if (ch == '\n') {
        for (uint64_t i = 0; i != amount; i++) {
            mmio_write(&device->dr_offset, '\r');
            wait_for_tx_complete(device);

            mmio_write(&device->dr_offset, ch);
            wait_for_tx_complete(device);
        }
    } else {
        for (uint64_t i = 0; i != amount; i++) {
            mmio_write(&device->dr_offset, ch);
            wait_for_tx_complete(device);
        }
    }
}

__debug_optimize(3) static
void pl011_send_sv(struct terminal *const term, const struct string_view sv) {
    struct pl011_device*const info =
        parent_of(term, struct pl011_device, term);

    volatile struct pl011_registers *const regs = info->regs;

    wait_for_tx_complete(regs);
    sv_foreach(sv, iter) {
        const char ch = *iter;
        if (ch == '\n') {
            mmio_write(&regs->dr_offset, '\r');
            wait_for_tx_complete(regs);
        }

        mmio_write(&regs->dr_offset, ch);
        wait_for_tx_complete(regs);
    }
}

#define PL011_BASE_CLOCK 0x16e3600

void
pl011_init(struct bus *const bus,
           const port_t base,
           const uint32_t baudrate,
           const uint32_t data_bits,
           const uint32_t stop_bits)
{
    struct pl011_device *info = nullptr;
    if (kmalloc_initialized()) {
        info = kmalloc(sizeof(*info));
        if (info == nullptr) {
            printk(LOGLEVEL_WARN, "pl011: failed to alloc info\n");
            return;
        }
    } else {
        if (early_info_count == countof(early_infos)) {
            printk(LOGLEVEL_WARN, "pl011: exhausted early-infos struct\n");
            return;
        }

        info = &early_infos[early_info_count];
        early_info_count++;
    }

    device_initialize(&info->device,
                      bus,
                      /*driver=*/nullptr,
                      SV_STATIC("pl011"));

    volatile struct pl011_registers *const regs =
        (volatile struct pl011_registers *)base;

    const uint32_t cr = mmio_read(&regs->cr_offset);
    uint32_t lcr = mmio_read(&regs->lcr_offset);

    // Disable UART before anything else
    mmio_write(&regs->cr_offset, cr & __CR_UARTEN);

    // Wait for any ongoing transmissions to complete
    wait_for_tx_complete(regs);

    // Flush FIFOs
    mmio_write(&regs->lcr_offset, rm_mask(lcr, __LCR_FEN));

    // Set frequency divisors (ibrd and fbrd) to configure the speed
    const uint32_t div = 4 * PL011_BASE_CLOCK / baudrate;

    const uint32_t ibrd = div & 0x3f;
    const uint32_t fbrd = (div >> 6) & 0xffff;

    mmio_write(&regs->ibrd_offset, ibrd);
    mmio_write(&regs->fbrd_offset, fbrd);

    // Configure data frame format according to the parameters (lcr_h).
    // We don't actually use all the possibilities, so this part of the code
    // can be simplified.
    lcr = 0x0;
    // WLEN part of lcr_h, you can check that this calculation does the
    // right thing for yourself
    lcr |= ((data_bits - 1) & 0x3) << 5;

    // Configure the number of stop bits
    if (stop_bits == 2) {
        lcr |= __LCR_STP2;
    }

    // Mask all interrupts by setting corresponding bits to 1
    mmio_write(&regs->imsc_offset, 0x7ff);

    // Disable DMA by setting all bits to 0
    mmio_write(&regs->dmacr_offset, 0x0);

    // I only need transmission, so that's the only thing I enabled.
    mmio_write(&regs->cr_offset, __CR_TXEN);

    // Finally enable UART
    mmio_write(&regs->cr_offset, __CR_TXEN | __CR_UARTEN);

    info->regs = regs;
    info->term.emit_ch = pl011_send_char,
    info->term.emit_sv = pl011_send_sv,

    printk_add_terminal(&info->term);
}

static bool pl011_dtb_probe(struct device *const device) {
    struct dtb_device *const dtb_device =
        parent_of(device, struct dtb_device, device);

    const struct devicetree_node *const node = dtb_device->node;
    const struct devicetree_prop_reg *const reg_prop =
        (const struct devicetree_prop_reg *)(uint64_t)
            devicetree_node_get_prop(node, DEVICETREE_PROP_REG);

    if (reg_prop == nullptr) {
        printk(LOGLEVEL_INFO, "pl031: dtb-node is missing a 'reg' prop\n");
        return false;
    }

    const struct array reg_list = reg_prop->list;
    if (array_empty(reg_list)) {
        printk(LOGLEVEL_INFO, "pl031: dtb-node has an empty 'reg' prop\n");
        return false;
    }

    struct devicetree_prop_reg_info *const reg_info =
        array_front(&reg_list, struct devicetree_prop_reg_info);

    struct range reg_range = RANGE_EMPTY();
    if (!range_create_and_verify(reg_info->address,
                                 reg_info->size,
                                 &reg_range))
    {
        printk(LOGLEVEL_INFO, "pl031: dtb-node's 'reg' prop range overflows\n");
        return false;
    }

    if (!range_align_out(reg_range, PAGE_SIZE, &reg_range)) {
        printk(LOGLEVEL_INFO,
               "pl031: failed to align to page-size range of 'reg' prop of dtb "
               "node\n");
        return false;
    }

    struct mmio_region *const mmio =
        vmap_mmio(reg_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (mmio == nullptr) {
        printk(LOGLEVEL_INFO,
               "pl031: failed to mmio-map range of 'reg' prop of dtb-node\n");
        return false;
    }

    const uint64_t offset = reg_info->address - reg_range.front;
    const port_t address = mmio->base + offset;

    ptrarr_foreach(early_infos, early_info_count, info) {
        if ((port_t)info->regs == address) {
            return true;
        }
    }

    pl011_init(device->bus,
               address,
               /*baudrate=*/115200,
               /*data_bits=*/8,
               /*stop_bits=*/1);

    return true;
}

static void init_drivers() {
    static const struct string_view compat_names[] = {
        SV_STATIC("arm,pl011"), SV_STATIC("arm,primecell")
    };

    static struct dtb_driver driver = {
        .match_flags = __DTB_DRIVER_MATCH_COMPAT,
        .compat_list = compat_names,
        .compat_count = countof(compat_names),
    };

    driver_initialize(&driver.driver,
                      dtb_bus(),
                      /*name=*/SV_STATIC("arm-pl011"),
                      pl011_dtb_probe,
                      /*remove=*/nullptr,
                      /*shutdown=*/nullptr,
                      /*suspend=*/nullptr,
                      /*resume=*/nullptr);
}

MAKE_DEV_INIT_FUNC(init_drivers);
