/* power.h - PlusOS CRT Phosphor Saver & Power Management Subsystem */
#ifndef PLUSOS_POWER_H
#define PLUSOS_POWER_H

#include <stdint.h>
#include <plusos/syscall.h>

void power_init(void);
void power_activity(void);
void power_vbl_tick(void);
void power_blank(int on);
int  power_status(struct plusos_power_args *args);

#endif
