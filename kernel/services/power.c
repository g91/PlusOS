/* power.c - PlusOS CRT Phosphor Saver & Power Management Subsystem */
#include "kernel.h"
#include "proc.h"
#include "power.h"

static uint32_t last_activity_tick = 0;
static uint32_t blank_timeout_ticks = 300 * 60; /* 5 minutes */
static int      is_blanked = 0;
static uint8_t *saved_screen = 0;

void power_init(void)
{
    last_activity_tick = tick_count();
    is_blanked = 0;
    saved_screen = 0;
}

void power_activity(void)
{
    last_activity_tick = tick_count();
    if (is_blanked) {
        power_blank(0);
    }
}

void power_blank(int on)
{
    uint8_t *scr = video_front();
    if (!scr) return;

    if (on && !is_blanked) {
        /* Save screen to back buffer or temporary stash */
        if (!saved_screen) {
            saved_screen = (uint8_t *)kmalloc(512 * 342 / 8);
        }
        if (saved_screen) {
            memcpy(saved_screen, scr, 512 * 342 / 8);
        }
        memset(scr, 0, 512 * 342 / 8); /* Blank to pure white/black raster */
        is_blanked = 1;
    } else if (!on && is_blanked) {
        if (saved_screen) {
            memcpy(scr, saved_screen, 512 * 342 / 8);
        }
        is_blanked = 0;
    }
}

void power_vbl_tick(void)
{
    if (blank_timeout_ticks > 0 && !is_blanked) {
        if (tick_count() - last_activity_tick >= blank_timeout_ticks) {
            power_blank(1);
        }
    }
}

int power_status(struct plusos_power_args *args)
{
    if (!args) return -1;
    switch (args->op) {
    case PLUSOS_PWR_STATUS:
        args->blank_on = is_blanked;
        args->timeout_ticks = (int)blank_timeout_ticks;
        args->idle_ticks = tick_count() - last_activity_tick;
        return 0;
    case PLUSOS_PWR_BLANK:
        power_blank(args->blank_on);
        return 0;
    case PLUSOS_PWR_SET_TIMEOUT:
        blank_timeout_ticks = (uint32_t)args->timeout_ticks;
        return 0;
    default:
        return -1;
    }
}
