/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Power Management & Soft-Power Egret Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_POWER_MGR_H
#define LC3_POWER_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define PM_NO_ERR         0
#define PM_PARAM_ERR    -50

typedef struct {
    bool hasSoftPower;
    bool isACPowered;
    uint8_t batteryLevel;
} PowerStatus;

/* Power Management API */
void lc3_power_off(void);
int16_t lc3_sleep(int16_t sleepMode);
void lc3_idle_update(void);
int16_t lc3_get_power_status(PowerStatus *status);

#endif /* LC3_POWER_MGR_H */
