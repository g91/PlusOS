/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Sonora Egret Soft Power-Down & Sleep Sequencer
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_EGRET_POWER_SLEEP_H
#define LC3_EGRET_POWER_SLEEP_H

#include <stdint.h>
#include <stdbool.h>

#define EGRET_NO_ERR         0
#define EGRET_TIMEOUT_ERR -3010
#define EGRET_PARAM_ERR     -50

typedef struct EgretPowerStatus {
    bool isACOn;
    bool isBatteryGood;
    bool isSoftPowerSupported;
    uint32_t wakeupAlarmSeconds;
} EgretPowerStatus;

/* Egret Power API */
int16_t lc3_egret_power_off(void);
int16_t lc3_egret_restart(void);
int16_t lc3_egret_set_wakeup_timer(uint32_t secondsFromNow);
int16_t lc3_egret_get_power_status(EgretPowerStatus *status);
void lc3_egret_enter_idle_mode(void);

#endif /* LC3_EGRET_POWER_SLEEP_H */
