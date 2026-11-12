/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: PowerBook / Portable Power & Battery Voltage Sensing Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_POWER_SENSE_H
#define LC3_POWER_SENSE_H

#include <stdint.h>
#include <stdbool.h>

#define PM_NO_ERR         0
#define PM_PARAM_ERR    -50

typedef struct BatteryStatus {
    uint8_t batteryLevel;   // 0-255 (255 = 100% full)
    bool isCharging;
    bool isACConnected;
    int16_t voltageMillivolts;
    int16_t currentMilliamps;
} BatteryStatus;

/* Power Sense API */
int16_t lc3_pm_get_battery_level(BatteryStatus *status);
int16_t lc3_pm_set_sleep_time(uint32_t seconds);
int16_t lc3_pm_check_charger(bool *chargerPluggedIn);
void lc3_pm_dim_backlight(uint8_t brightness);
void lc3_pm_undim_backlight(void);

#endif /* LC3_POWER_SENSE_H */
