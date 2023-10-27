/*
 * kernel/dev/uart/8250.c
 * © suhas pai
 */

#include "dev/dtb/dtb.h"
#include "cpu/spinlock.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "mm/kmalloc.h"
#include "8250.h"

#define UART_RBR_OFFSET 0  // In:  Recieve Buffer Register
#define UART_THR_OFFSET 0  // Out: Transmitter Holding Register
#define UART_DLL_OFFSET 0  // Out: Divisor Latch Low
#define UART_IER_OFFSET 1  // I/O: Interrupt Enable Register
#define UART_DLM_OFFSET 1  // Out: Divisor Latch High
#define UART_FCR_OFFSET 2  // Out: FIFO Control Register
#define UART_IIR_OFFSET 2  // I/O: Interrupt Identification Register
#define UART_LCR_OFFSET 3  // Out: Line Control Register
#define UART_MCR_OFFSET 4  // Out: Modem Control Register
#define UART_LSR_OFFSET 5  // In:  Line Status Register
#define UART_MSR_OFFSET 6  // In:  Modem Status Register
#define UART_SCR_OFFSET 7  // I/O: Scratch Register
#define UART_MDR1_OFFSET 8  // I/O:  Mode Register

#define UART_LSR_FIFOE 0x80 // Fifo error
#define UART_LSR_TEMT 0x40  // Transmitter empty
#define UART_LSR_THRE 0x20  // Transmit-hold-register empty
#define UART_LSR_BI 0x10    // Break interrupt indicator
#define UART_LSR_FE 0x08    // Frame error indicator
#define UART_LSR_PE 0x04    // Parity error indicator
#define UART_LSR_OE 0x02    // Overrun error indicator
#define UART_LSR_DR 0x01    // Receiver data ready
#define UART_LSR_BRK_ERROR_BITS 0x1E    // BI, FE, PE, OE bits

struct uart8250_info {
    struct terminal term;
    struct spinlock lock;

    port_t base;

    uint32_t in_freq;
    uint32_t reg_width;
    uint32_t reg_shift;
};

// for use when initializing serial before mm/kmalloc
static struct uart8250_info early_infos[8] = {};
static uint16_t early_info_count = 0;

static inline uint32_t
get_reg(const port_t uart8250_base,
        struct uart8250_info *const info,
        const uint32_t num)
{
    const uint32_t offset = num << info->reg_shift;
    switch (info->reg_width) {
        case sizeof(uint8_t):
            return port_in8(uart8250_base + offset);
        case sizeof(uint16_t):
            return port_in16(uart8250_base + offset);
        case sizeof(uint32_t):
            return port_in32(uart8250_base + offset);
    }

    verify_not_reached();
}

static inline void
set_reg(const port_t uart8250_base,
        struct uart8250_info *const info,
        const uint32_t num,
        const uint32_t val)
{
    const uint32_t offset = num << info->reg_shift;
    switch (info->reg_width) {
        case sizeof(uint8_t):
            port_out8(uart8250_base + offset, val);
            return;
        case sizeof(uint16_t):
            port_out16(uart8250_base + offset, val);
            return;
        case sizeof(uint32_t):
            port_out32(uart8250_base + offset, val);
            return;
    }

    verify_not_reached();
}

#define MAX_ATTEMPTS 10

__optimize(3) static void
uart8250_putc(const port_t base,
              struct uart8250_info *const info,
              const char ch)
{
    for (uint64_t i = 0; i != MAX_ATTEMPTS; i++) {
        if (get_reg(base, info, UART_LSR_OFFSET) & UART_LSR_THRE) {
            set_reg(base, info, UART_THR_OFFSET, ch);
            return;
        }
    }
}

__optimize(3) static void
uart8250_send_char(struct terminal *const term,
                   const char ch,
                   const uint32_t amount)
{
    struct uart8250_info *const info = (struct uart8250_info *)term;
    const bool flag = spin_acquire_with_irq(&info->lock);

    for (uint32_t i = 0; i != amount; i++) {
        uart8250_putc(info->base, info, ch);
    }

    spin_release_with_irq(&info->lock, flag);
}

__optimize(3) static void
uart8250_send_sv(struct terminal *const term, const struct string_view sv) {
    struct uart8250_info *const info = (struct uart8250_info *)term;
    const bool flag = spin_acquire_with_irq(&info->lock);

    sv_foreach(sv, iter) {
        uart8250_putc(info->base, info, *iter);
    }

    spin_release_with_irq(&info->lock, flag);
}

bool
uart8250_init(const port_t base,
              const uint32_t baudrate,
              const uint32_t in_freq,
              const uint8_t reg_width,
              const uint8_t reg_shift)
{
    uint16_t bdiv = 0;
    if (baudrate != 0) {
        bdiv = (in_freq + 8 * baudrate) / (16 * baudrate);
    }

    struct uart8250_info *info = NULL;
    if (kmalloc_initialized()) {
        info = kmalloc(sizeof(*info));
        if (info == NULL) {
            printk(LOGLEVEL_WARN, "uart8250: failed to alloc info\n");
            return false;
        }
    } else {
        if (early_info_count == countof(early_infos)) {
            printk(LOGLEVEL_WARN, "uart8250: exhausted early-infos struct\n");
            return false;
        }

        info = &early_infos[early_info_count];
        early_info_count++;
    }

    info->base = base;
    info->in_freq = in_freq;

    info->reg_width = reg_width;
    info->reg_shift = reg_shift;

    // Disable all interrupts
    set_reg(base, info, UART_IER_OFFSET, 0x00);
    // Enable DLAB
    set_reg(base, info, UART_LCR_OFFSET, 0x80);

    if (bdiv != 0) {
        // Set divisor low byte
        set_reg(base, info, UART_DLL_OFFSET, bdiv & 0xff);
        // Set divisor high byte
        set_reg(base, info, UART_DLM_OFFSET, (bdiv >> 8) & 0xff);
    }

    // 8 bits, no parity, one stop bit
    set_reg(base, info, UART_LCR_OFFSET, 0x03);
    // Enable FIFO
    set_reg(base, info, UART_FCR_OFFSET, 0x01);
    // No modem control DTR RTS
    set_reg(base, info, UART_MCR_OFFSET, 0x00);
    // Clear line status
    get_reg(base, info, UART_LSR_OFFSET);
    // Read receive buffer
    get_reg(base, info, UART_RBR_OFFSET);
    // Set scratchpad
    set_reg(base, info, UART_SCR_OFFSET, 0x00);

    info->lock = SPINLOCK_INIT();

    info->term.emit_ch = uart8250_send_char;
    info->term.emit_sv = uart8250_send_sv;

    printk_add_terminal(&info->term);
    return true;
}

bool init_from_dtb(const void *const dtb, const int nodeoff) {
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
               "uart8250: base-addr reg of 'reg' property of serial dtb-node "
               "is malformed\n");
        return false;
    }

    struct string_view clock_freq_string = SV_EMPTY();
    const bool get_clock_freq_result =
        dtb_get_string_prop(dtb,
                            nodeoff,
                            "clock-frequency",
                            &clock_freq_string);

    if (!get_clock_freq_result) {
        printk(LOGLEVEL_WARN,
               "uart8250: clock-frequency property of serial dtb-node is "
               "missing or malformed");
        return false;
    }

    uart8250_init((port_t)base_addr_reg.address,
                  /*baudrate=*/115200,
                  *(const uint32_t *)(uint64_t)clock_freq_string.begin,
                  /*reg_width=*/sizeof(uint8_t),
                  /*reg_shift=*/0);
    return true;
}

static const char *const dtb_compat_list[] = { "ns16550a" };
static struct dtb_driver dtb_driver = {
    .init = init_from_dtb,
    .compat_list = dtb_compat_list,
    .compat_count = countof(dtb_compat_list)
};

__driver static const struct driver uart8250_driver = {
    .dtb = &dtb_driver,
    .pci = NULL
};