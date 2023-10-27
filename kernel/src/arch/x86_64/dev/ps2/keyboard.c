/*
 * kernel/arch/x86_64/dev/ps2/keyboard.c
 * © suhas pai
 */

#include "dev/ps2/keymap.h"
#include "lib/adt/string.h"

#include "asm/irqs.h"
#include "cpu/isr.h"

#include "dev/printk.h"
#include "lib/util.h"

#include "keyboard.h"

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

    /*
     * Sets whether the current key was released, or is from extension
     * scan-code.
     */

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

static isr_vector_t g_ps2_vector = 0;
static char get_char_from_ps2_kb(const uint8_t scan_code) {
    if (g_kbd_state.shift != 0) {
        return ps2_key_to_char_shift[scan_code];
    }

    if (g_kbd_state.caps_lock) {
        return ps2_key_to_char_capslock[scan_code];
    }

    return ps2_key_to_char[scan_code];
}

void
ps2_keyboard_interrupt(const uint64_t int_no, irq_context_t *const context) {
    (void)int_no;
    (void)context;

    const uint8_t scan_code = ps2_read_input_byte();
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

    struct string string = STRING_EMPTY();
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
        if (g_kbd_state.shift != 0 ||
            g_kbd_state.ctrl != 0 ||
            g_kbd_state.alt)
        {
            string_append_sv(&string, SV_STATIC("-"));
        }

        string_append_sv(&string, SV_STATIC("cmd"));
    }

    printk(LOGLEVEL_WARN,
           "ps2: " STRING_FMT " '%c'%s\n",
           STRING_FMT_ARGS(string),
           get_char_from_ps2_kb(scan_code),
           g_kbd_state.caps_lock ? " [caps-lock]" : "");

    string_destroy(&string);
}

void ps2_keyboard_init(const enum ps2_port_id device_id) {
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

    g_ps2_vector = isr_alloc_vector();

    isr_set_vector(g_ps2_vector, ps2_keyboard_interrupt, &ARCH_ISR_INFO_NONE());
    isr_assign_irq_to_cpu(get_cpu_info_mut(),
                          IRQ_KEYBOARD,
                          g_ps2_vector,
                          /*masked=*/false);

    if (ps2_read_input_byte() != -1) {
        printk(LOGLEVEL_INFO, "ps2: keyboard initialized\n");
    } else {
        printk(LOGLEVEL_INFO, "ps2: keyboard failed to initialize\n");
    }
}