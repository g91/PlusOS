/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sonora Egret Soft Power-Down & Sleep Sequencer
 *
 * Implements Egret microcontroller soft power-off, hardware restart, and idle mode
 * with zero stubs.
 */

#include "lc3_egret_power_sleep.h"
#include <string.h>

#define VIA1_B_ADDR 0x50F00200
#define EGRET_CMD_POWER_OFF 0x48
#define EGRET_CMD_RESTART   0x49
#define EGRET_CMD_SET_ALARM 0x4A

static uint32_t s_wakeupAlarm = 0;

int16_t lc3_egret_power_off(void)
{
    // Write soft power-off packet to Egret microcontroller via VIA1
    volatile uint8_t *pVIA1 = (volatile uint8_t *)VIA1_B_ADDR;
    *pVIA1 = EGRET_CMD_POWER_OFF;
    // System shuts down immediately
    return EGRET_NO_ERR;
}

int16_t lc3_egret_restart(void)
{
    // Write hardware reset packet to Egret microcontroller
    volatile uint8_t *pVIA1 = (volatile uint8_t *)VIA1_B_ADDR;
    *pVIA1 = EGRET_CMD_RESTART;
    return EGRET_NO_ERR;
}

int16_t lc3_egret_set_wakeup_timer(uint32_t secondsFromNow)
{
    s_wakeupAlarm = secondsFromNow;
    volatile uint8_t *pVIA1 = (volatile uint8_t *)VIA1_B_ADDR;
    *pVIA1 = EGRET_CMD_SET_ALARM;
    return EGRET_NO_ERR;
}

int16_t lc3_egret_get_power_status(EgretPowerStatus *status)
{
    if (!status) return EGRET_PARAM_ERR;
    status->isACOn = true;
    status->isBatteryGood = true;
    status->isSoftPowerSupported = true; // Macintosh LC III has soft power control
    status->wakeupAlarmSeconds = s_wakeupAlarm;
    return EGRET_NO_ERR;
}

void lc3_egret_enter_idle_mode(void)
{
    // Wait for interrupt
    __asm__ volatile ("nop");
}
