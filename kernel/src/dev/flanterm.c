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

#if defined(DISABLE_FLANTERM)
    __unused
#endif /* defined(DISABLE_FLANTERM) */

__optimize(3) static void
flanterm_write_char(struct terminal *const term,
                    const char ch,
                    const uint32_t amt)
{
    struct flanterm_fb_info *const fb_info = (struct flanterm_fb_info *)term;
    for (uint32_t i = 0; i != amt; i++) {
        flanterm_write(fb_info->ctx, &ch, 1);
    }
}

#if defined(DISABLE_FLANTERM)
    __unused
#endif /* defined(DISABLE_FLANTERM) */

__optimize(3) static void
flanterm_write_sv(struct terminal *const term, const struct string_view sv) {
    struct flanterm_fb_info *const fb_info = (struct flanterm_fb_info *)term;
    flanterm_write(fb_info->ctx, sv.begin, sv.length);
}

#if defined(DISABLE_FLANTERM)
    __unused
#endif /* defined(DISABLE_FLANTERM) */
static struct flanterm_fb_info g_fb_info_list[64] = {};

void setup_flanterm() {
#if defined(DISABLE_FLANTERM)
    return;
#else
    // Fetch the first framebuffer.
    const uint64_t fb_count =
        min(boot_get_fb()->framebuffer_count, countof(g_fb_info_list));

    if (fb_count == 0) {
        return;
    }

    for (uint64_t i = 0; i != fb_count; i++) {
        const struct limine_framebuffer *const framebuffer =
            boot_get_fb()->framebuffers[i];

        struct flanterm_context *const context =
            flanterm_fb_init(/*_malloc=*/NULL,
                             /*_free=*/NULL,
                             framebuffer->address,
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

        if (context == NULL) {
            printk(LOGLEVEL_WARN, "flanterm: failed to init\n");
            continue;
        }

        g_fb_info_list[i].ctx = context;
        g_fb_info_list[i].terminal.emit_ch = flanterm_write_char;
        g_fb_info_list[i].terminal.emit_sv = flanterm_write_sv;

        printk(LOGLEVEL_INFO,
               "flanterm: framebuffer %" PRIu64 " successfully setup\n",
               i);

        printk_add_terminal(&g_fb_info_list[i].terminal);
    }
#endif /* defined(DISABLE_FLANTERM) */
}