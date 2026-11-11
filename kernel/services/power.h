/* power.h - PlusOS CRT Phosphor Saver & Power Management Subsystem */
#ifndef PLUSOS_POWER_H
#define PLUSOS_POWER_H

#include <stdint.h>
#include <plusos/syscall.h>
#include "kconfig.h"

#if CONFIG_POWER
void power_init(void);
void power_activity(void);
void power_vbl_tick(void);
void power_blank(int on);
int  power_status(struct plusos_power_args *args);
#else
static inline void power_init(void) {}
static inline void power_activity(void) {}
static inline void power_vbl_tick(void) {}
static inline void power_blank(int on) { (void)on; }
static inline int  power_status(struct plusos_power_args *a) { (void)a; return -1; }
#endif

#endif