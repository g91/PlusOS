/* power.c - PlusOS CRT Phosphor Saver & Power Management Subsystem */
#include "kernel.h"
#include "proc.h"
#include "power.h"

static uint32_t last_activity_tick = 0;
static uint32_t blank_timeout_ticks = 0; /* 0 = off by default; safe at boot */
static int      is_blanked = 0;

void power_init(void)
{
    last_activity_tick = tick_count();
    is_blanked = 0;
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
    uint8_t *bk = video_back();
    if (!scr) return;

    uint32_t fb_sz = video_bytes();
    if (!fb_sz) fb_sz = 512 * 342 / 8;

    if (on && !is_blanked) {
        if (bk && bk != scr) {
            memcpy(bk, scr, fb_sz);
        }
        memset(scr, 0, fb_sz);
        is_blanked = 1;
    } else if (!on && is_blanked) {
        if (bk && bk != scr) {
            memcpy(scr, bk, fb_sz);
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
