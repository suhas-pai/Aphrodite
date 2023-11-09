/*
 * kernel/src/dev/flanterm.c
 * © suhas pai
 */

#include "flanterm/backends/fb.h"
#include "flanterm/flanterm.h"

#include "dev/printk.h"
#include "sys/boot.h"

struct flanterm_fb_info {
    struct terminal terminal;
    struct flanterm_context *ctx;
};

static void
flanterm_write_char(struct terminal *const term,
                    const char ch,
                    const uint32_t amt)
{
    struct flanterm_fb_info *const fb_info = (struct flanterm_fb_info *)term;
    for (uint32_t i = 0; i != amt; i++) {
        flanterm_write(fb_info->ctx, &ch, 1);
    }
}

static void
flanterm_write_sv(struct terminal *const term, const struct string_view sv) {
    struct flanterm_fb_info *const fb_info = (struct flanterm_fb_info *)term;
    flanterm_write(fb_info->ctx, sv.begin, sv.length);
}

static struct flanterm_fb_info fb_info = {
    .terminal.emit_ch = flanterm_write_char,
    .terminal.emit_sv = flanterm_write_sv,
};

void setup_flanterm() {
    // Fetch the first framebuffer.
    const struct limine_framebuffer *const framebuffer =
        boot_get_fb()->framebuffers[0];

    uint32_t *const fb_ptr = framebuffer->address;
    struct flanterm_context *const context =
        flanterm_fb_init(/*_malloc=*/NULL,
                         /*_free=*/NULL,
                         fb_ptr,
                         framebuffer->width,
                         framebuffer->height,
                         framebuffer->pitch,
                    #if defined(FLANTERM_FB_SUPPORT_BPP)
                         framebuffer->red_mask_size,
                         framebuffer->red_mask_shift,
                         framebuffer->green_mask_size,
                         framebuffer->green_mask_shift,
                         framebuffer->blue_mask_size,
                         framebuffer->blue_mask_shift,
                    #endif /* !defined(FLANTERM_FB_SUPPORT_BPP) */
                    #if !defined(FLANTERM_FB_DISABLE_CANVAS)
                         /*canvas=*/NULL,
                    #endif /* !defined(FLANTERM_FB_DISABLE_CANVAS) */
                         /*ansi_colours=*/NULL,
                         /*ansi_bright_colours=*/NULL,
                         /*default_bg=*/NULL,
                         /*default_fg=*/NULL,
                         /*default_bg_bright=*/NULL,
                         /*default_fg_bright=*/NULL,
                         /*font=*/NULL,
                         /*font_width=*/0,
                         /*font_height=*/0,
                         /*font_spacing=*/1,
                         /*font_scale_x=*/1,
                         /*font_scale_y=*/1,
                         /*margin=*/0);

    fb_info.ctx = context;

    printk_add_terminal(&fb_info.terminal);
    printk(LOGLEVEL_INFO, "flanterm setup");
}