/* sys7.h - Extended OS Subsystems for PlusOS.
 *
 * Implements clean Macintosh OS manager parity on top of PlusOS & Mac ROMs:
 *   - Gestalt Manager (_Gestalt selector registry + ROM fallback)
 *   - ShutDown Manager (ShutDwnInstall / ShutDwnRemove / soft power-off)
 *   - Extended Time Manager (ms & microsecond precision queue + tick hook)
 *   - Deferred Task Manager (DTInstall FIFO queue drained safely)
 *   - Notification Manager (NMInstall / NMRemove / notification queue)
 */
#ifndef PLUSOS_SYS7_H
#define PLUSOS_SYS7_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*sys7_gestalt_fn)(uint32_t selector, int32_t *response);
typedef void (*sys7_shutdown_fn)(void *arg, int is_restart);
typedef void (*sys7_timer_fn)(void *arg);
typedef void (*sys7_dtask_fn)(void *arg);

#define SYS7_SD_POWEROFF  0x01
#define SYS7_SD_RESTART   0x02

void sys7_init(void);
void sys7_vbl_tick(void);           /* called at 60 Hz from tick_c() */
void sys7_poll(void);               /* called on idle / safe kernel points */

/* Gestalt Manager */
int  sys7_gestalt(uint32_t selector, int32_t *response);
int  sys7_new_gestalt(uint32_t selector, sys7_gestalt_fn fn);

/* ShutDown Manager */
int  sys7_shutdown_install(sys7_shutdown_fn fn, void *arg, int flags);
int  sys7_shutdown_remove(sys7_shutdown_fn fn);
void sys7_run_shutdown_hooks(int is_restart);

/* Extended Time Manager (delay > 0 = ms, delay < 0 = -us; returns id or -1) */
int  sys7_time_install(sys7_timer_fn fn, void *arg);
int  sys7_time_prime(int id, int32_t delay);
int  sys7_time_remove(int id);

/* Deferred Task Manager */
int  sys7_dt_install(sys7_dtask_fn fn, void *arg);
int  sys7_dt_pending(void);

/* Notification Manager */
int  sys7_nm_post(const char *msg, int beep);
int  sys7_nm_poll(char *buf, int buf_sz);
int  sys7_nm_count(void);

#ifdef __cplusplus
}
#endif

#endif /* PLUSOS_SYS7_H */
