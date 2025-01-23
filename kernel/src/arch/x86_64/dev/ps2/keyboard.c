/*
 * kernel/src/arch/x86_64/dev/ps2/keyboard.c
 * © suhas pai
 */

#include <lib/adt/string.h>
#include <lib/util.h>

#include "dev/ps2/keymap.h"
#include "dev/ps2/keyboard.h"

#include "acpi/bus.h"
#include "acpi/device.h"
#include "acpi/driver.h"

#include "asm/irqs.h"

#include "dev/init.h"
#include "dev/printk.h"

const char ps2_key_to_char[PS2_KEYMAP_SIZE] = {
    '\0', '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n', '\0', 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    '\0', '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', '\0', '\0',
    '\0', ' '
};

const char ps2_key_to_char_shift[PS2_KEYMAP_SIZE] = {
    '\0', '\e', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', '\0', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    '\0', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '\0', '\0',
    '\0', ' '
};

const char ps2_key_to_char_capslock[PS2_KEYMAP_SIZE] = {
    '\0', '\e', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n', '\0', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    '\0', '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', '\0', '\0',
    '\0', ' '
};

struct ps2_keyboard_state {
    // We can have 2 cmd keys, 2 alt keys, 2 shift keys, and 2 ctrl keys.

    uint8_t shift : 2;
    uint8_t cmd : 2;
    uint8_t ctrl : 2;

    // Sets whether the current key was released, or is from extension
    // scan-code.

    bool alt : 1;
    bool caps_lock : 1;
    bool in_e0 : 1;
};

static struct ps2_keyboard_state g_kbd_state = {
    .shift = 0,
    .cmd = 0,
    .ctrl = 0,
    .alt = 0,
    .caps_lock = 0,
    .in_e0 = false
};

__debug_optimize(3) static char ps2_keyboard_get_char(const uint8_t scan_code) {
    if (g_kbd_state.shift != 0) {
        return ps2_key_to_char_shift[scan_code];
    }

    if (g_kbd_state.caps_lock) {
        return ps2_key_to_char_capslock[scan_code];
    }

    return ps2_key_to_char[scan_code];
}

void
ps2_keyboard_interrupt(const uint64_t intr_no,
                       struct thread_context *const context,
                       void *const ctx)
{
    (void)intr_no;
    (void)context;
    (void)ctx;

    const uint8_t scan_code = ps2_read_input_byte();
    isr_eoi(intr_no);

    if (g_kbd_state.in_e0) {
        g_kbd_state.in_e0 = false;
        switch ((enum ps2_scancode_e0_keys)scan_code) {
            case PS2_SCANNODE_E0_LEFT_CMD:
            case PS2_SCANNODE_E0_RIGHT_CMD:
                g_kbd_state.cmd += 1;
                return;
            case PS2_SCANNODE_E0_LEFT_CMD_REL:
            case PS2_SCANNODE_E0_RIGHT_CMD_REL:
                g_kbd_state.cmd -= 1;
                return;
            case PS2_SCANNODE_E0_RIGHT_CTRL:
                g_kbd_state.ctrl += 1;
                return;
            case PS2_SCANNODE_E0_RIGHT_CTRL_REL:
                g_kbd_state.ctrl -= 1;
                return;
            case PS2_SCANNODE_E0_UP_ARROW:
                printk(LOGLEVEL_INFO, "ps2: up-arrow\n");
                return;
            case PS2_SCANNODE_E0_LEFT_ARROW:
                printk(LOGLEVEL_INFO, "ps2: left-arrow\n");
                return;
            case PS2_SCANNODE_E0_RIGHT_ARROW:
                printk(LOGLEVEL_INFO, "ps2: right-arrow\n");
                return;
            case PS2_SCANNODE_E0_DOWN_ARROW:
                printk(LOGLEVEL_INFO, "ps2: down-arrow\n");
                return;
            case PS2_SCANNODE_E0_UP_ARROW_REL:
            case PS2_SCANNODE_E0_LEFT_ARROW_REL:
            case PS2_SCANNODE_E0_RIGHT_ARROW_REL:
            case PS2_SCANNODE_E0_DOWN_ARROW_REL:
                return;
        }

        printk(LOGLEVEL_WARN,
               "ps2: unrecognized e0 scan-code 0x%" PRIx8 "\n",
               scan_code);
        return;
    }

    switch ((enum ps2_scancode_keys)scan_code) {
        case PS2_SCANNODE_E0:
            g_kbd_state.in_e0 = true;
            return;
        case PS2_SCANCODE_CTRL:
            g_kbd_state.ctrl += 1;
            return;
        case PS2_SCANCODE_CTRL_REL:
            g_kbd_state.ctrl -= 1;
            return;
        case PS2_SCANCODE_SHIFT_LEFT:
        case PS2_SCANCODE_SHIFT_RIGHT:
            g_kbd_state.shift += 1;
            return;
        case PS2_SCANCODE_SHIFT_LEFT_REL:
        case PS2_SCANCODE_SHIFT_RIGHT_REL:
            g_kbd_state.shift -= 1;
            return;
        case PS2_SCANCODE_ALT_LEFT:
            g_kbd_state.alt = true;
            return;
        case PS2_SCANCODE_ALT_LEFT_REL:
            g_kbd_state.alt = false;
            return;
        case PS2_SCANCODE_CAPSLOCK:
            g_kbd_state.caps_lock = !g_kbd_state.caps_lock;
            return;
        case PS2_SCANCODE_NUMLOCK:
            printk(LOGLEVEL_INFO, "ps2: got numlock\n");
            return;
    }

    if (scan_code == PS2_RESPONSE_ACKNOWLEDGE) {
        return;
    }

    if (scan_code & __PS2_KBD_KEY_RELEASE) {
        return;
    }

    if (!index_in_bounds(scan_code, countof(ps2_key_to_char))) {
        printk(LOGLEVEL_WARN,
               "ps2: kbd scan code: 0x%" PRIx8 " (not in array)\n", scan_code);
        return;
    }

    struct string string = STRING_NULL();
    if (g_kbd_state.shift != 0) {
        string_append_sv(&string, SV_STATIC("shift"));
    }

    if (g_kbd_state.ctrl != 0) {
        if (g_kbd_state.shift != 0) {
            string_append_sv(&string, SV_STATIC("-"));
        }

        string_append_sv(&string, SV_STATIC("ctrl"));
    }

    if (g_kbd_state.alt) {
        if (g_kbd_state.shift != 0 || g_kbd_state.ctrl != 0) {
            string_append_sv(&string, SV_STATIC("-"));
        }

        string_append_sv(&string, SV_STATIC("alt"));
    }

    if (g_kbd_state.cmd != 0) {
        if (g_kbd_state.shift != 0 || g_kbd_state.ctrl != 0 || g_kbd_state.alt)
        {
            string_append_sv(&string, SV_STATIC("-"));
        }

        string_append_sv(&string, SV_STATIC("cmd"));
    }

    printk(LOGLEVEL_WARN,
           "ps2: " STRING_FMT " '%c'%s\n",
           STRING_FMT_ARGS(string),
           ps2_keyboard_get_char(scan_code),
           g_kbd_state.caps_lock ? " [caps-lock]" : "");

    string_destroy(&string);
}

void ps2_keyboard_start(const enum ps2_port_id device_id) {
    ps2_send_to_port(device_id, PS2_KBD_CMD_SCAN_CODE_SET);
    const int16_t get_response =
        ps2_send_to_port(device_id, PS2_KBD_SCAN_CODE_SET_SUBCMD_GET);

    if (get_response != PS2_RESPONSE_ACKNOWLEDGE) {
        printk(LOGLEVEL_WARN,
               "ps2: failed to get 'ACK' from keyboard scan-code get\n");
        return;
    }

    const int16_t scan_code = ps2_read_input_byte();
    if (scan_code != 0x41) {
        printk(LOGLEVEL_WARN,
               "ps2: Wrong scan-code set, got: %" PRIx16 "\n",
               scan_code);
        return;
    }

    struct irq_pin *const pin = isr_get_irq_pin(IRQ_KEYBOARD);
    if (!isr_install_irq(pin,
                         ps2_keyboard_interrupt,
                         /*ctx=*/nullptr,
                         /*masked=*/false))
    {
        printk(LOGLEVEL_WARN, "ps2: failed to install keyboard irq\n");
        return;
    }

    printk(LOGLEVEL_INFO, "ps2: keyboard initialized\n");
}

bool ps2_keyboard_probe(struct device *const the_device) {
    struct acpi_device *const device =
        parent_of(the_device, struct acpi_device, device);

    const struct os_acpi_device_resources *const resources = &device->resources;
    array_foreach(&resources->io_list, const struct os_acpi_io_info, io) {
        const char *decode_kind = "unknown";
        switch (io->decode_kind) {
            case OS_ACPI_IO_DECODE_KIND_16_BIT:
                decode_kind = "16-bit";
                break;
            case OS_ACPI_IO_DECODE_KIND_10_BIT:
                decode_kind = "10-bit";
                break;
        }

        printk(LOGLEVEL_INFO,
               "ps2/keyboard: found io port:\n"
               "\tdecode-kind: %s\n"
               "\tminimum: 0x%" PRIx16 "\n"
               "\tmaximum: 0x%" PRIx16 "\n"
               "\talignment: %" PRIx8 "\n"
               "\tlength: %" PRIx8 "\n",
               decode_kind,
               io->minimum,
               io->maximum,
               io->alignment,
               io->length);
    }

    array_foreach(&resources->irq_list, const struct os_acpi_irq_info, irq) {
        printk(LOGLEVEL_INFO,
               "ps2/keyboard: found irq:\n"
               "\ttrigger: %s\n"
               "\tlevel: %s\n"
               "\tshared: %s\n"
               "\twake capable: %s\n"
               "\t%" PRIu32 " irqs:\n",
               irq->trigger_mode == IRQ_TRIGGER_MODE_EDGE ? "edge" : "level",
               irq->polarity == IRQ_POLARITY_HIGH ? "high" : "low",
               irq->is_shared ? "yes" : "no",
               irq->wake_capable ? "yes" : "no",
               irq->irq_count);

        if (irq->irq_count == 0) {
            printk(LOGLEVEL_INFO, "\t\t[none]\n");
            continue;
        }

        if (irq->irq_count > 1) {
            printk(LOGLEVEL_INFO, "\t\t[shared]\n");
        }

        ptrarr_foreach(irq->irq_list, irq->irq_count, irq_num) {
            printk(LOGLEVEL_INFO, "\t\tirq %" PRIu32 "\n", *irq_num);

            struct irq_pin *const pin = isr_get_irq_pin(*irq_num);
            if (pin == nullptr) {
                printk(LOGLEVEL_WARN,
                       "ps2/keyboard: irq " IRQ_NUMBER_FMT " not found\n",
                       *irq_num);
                continue;
            }

            irq_pin_setup(pin, irq->polarity, irq->trigger_mode);
        }
    }

    const port_t input_buffer_port =
        array_front(&resources->io_list, const struct os_acpi_io_info)->minimum;
    const port_t read_status_port =
        array_at(&resources->io_list, const struct os_acpi_io_info, 1)->minimum;

    const irq_number_t keyboard_irq =
        array_front(&resources->irq_list, const struct os_acpi_irq_info)
            ->irq_list[0];

    ps2_init_keyboard(read_status_port, input_buffer_port, keyboard_irq);
    return true;
}

static const struct string_view pnp_ids[] = {
    SV_STATIC("PNP0300"),
    SV_STATIC("PNP0301"),
    SV_STATIC("PNP0302"),
    SV_STATIC("PNP0303"),
    SV_STATIC("PNP0304"),
    SV_STATIC("PNP0305"),
    SV_STATIC("PNP0306"),
    SV_STATIC("PNP0307"),
    SV_STATIC("PNP0308"),
    SV_STATIC("PNP0309"),
    SV_STATIC("PNP030A"),
    SV_STATIC("PNP030B"),
    SV_STATIC("PNP0320"),
    SV_STATIC("PNP0321"),
    SV_STATIC("PNP0322"),
    SV_STATIC("PNP0323"),
    SV_STATIC("PNP0324"),
    SV_STATIC("PNP0325"),
    SV_STATIC("PNP0326"),
    SV_STATIC("PNP0327"),
    SV_STATIC("PNP0340"),
    SV_STATIC("PNP0341"),
    SV_STATIC("PNP0342"),
    SV_STATIC("PNP0343"),
    SV_STATIC("PNP0343"),
    SV_STATIC("PNP0344"),
};

static uacpi_namespace_node *get_namespace() {
    return uacpi_namespace_get_predefined(UACPI_PREDEFINED_NAMESPACE_SB);
}

static void init_keyboard_driver() {
    static struct acpi_driver acpi_driver = {
        .pnp_ids = pnp_ids,
        .pnp_id_count = countof(pnp_ids),
        .resources_flags = ACPI_DRIVER_RESOURCES_IRQ | ACPI_DRIVER_RESOURCES_IO,
        .get_namespace = get_namespace,
    };

    driver_initialize(&acpi_driver.driver,
                      acpi_bus(),
                      /*name=*/SV_STATIC("ps2-keyboard"),
                      ps2_keyboard_probe,
                      /*remove=*/nullptr,
                      /*shutdown=*/nullptr,
                      /*suspend=*/nullptr,
                      /*resume=*/nullptr);
};

MAKE_DEV_INIT_FUNC(init_keyboard_driver);
