/* kconfig.h - compile-time kernel configuration.
 *
 * Every optional kernel module has a CONFIG_<NAME> switch (1 = built in, 0 =
 * left out). The Makefile sets them from the build PROFILE / DISABLE / ENABLE
 * variables (see config.mk) with -DCONFIG_<NAME>=0, and also drops the source
 * file of a disabled module from the link, so a leaner kernel is smaller, boots
 * faster, and has no dead code in the syscall / tick hot paths.
 *
 * A disabled module's syscall returns -1 (the kernel's usual "unsupported"
 * result) and its hooks compile to nothing.
 *
 * Defaults below apply when the Makefile does not override them. */
#ifndef PLUSOS_KCONFIG_H
#define PLUSOS_KCONFIG_H

/* ---- optional drivers ---- */
#ifndef CONFIG_SOUND
#define CONFIG_SOUND      1     /* drivers/sound: beep/tone/notes, SYS_sound */
#endif

/* ---- optional services ---- */
#ifndef CONFIG_PROFILER
#define CONFIG_PROFILER   1     /* services/profiler: switch/syscall counters */
#endif
#ifndef CONFIG_ENTROPY
#define CONFIG_ENTROPY    1     /* services/entropy: SYS_getrandom */
#endif
#ifndef CONFIG_POWER
#define CONFIG_POWER      1     /* services/power: screen saver, SYS_power */
#endif
#ifndef CONFIG_PRAM
#define CONFIG_PRAM       1     /* services/pram: parameter RAM, SYS_pram */
#endif
#ifndef CONFIG_SHM
#define CONFIG_SHM        1     /* services/shm: shared memory, SYS_shm */
#endif
#ifndef CONFIG_RAMDISK
#define CONFIG_RAMDISK    1     /* services/ramdisk: SYS_ramdisk */
#endif
#ifndef CONFIG_SCRAP
#define CONFIG_SCRAP      1     /* services/scrap: clipboard, SYS_scrap */
#endif
#ifndef CONFIG_MACLOADER
#define CONFIG_MACLOADER  1     /* services/macloader: classic 'APPL' programs */
#endif
#ifndef CONFIG_USERS
#define CONFIG_USERS      1     /* services/users: /etc/passwd, /etc/shadow, login */
#endif
#ifndef CONFIG_PERMS
#define CONFIG_PERMS      1     /* services/perms: /etc/perms.db, chmod/chown */
#endif

/* ---- robustness features (small, but each can be turned off for speed) ---- */
#ifndef CONFIG_STACKGUARD
#define CONFIG_STACKGUARD 1     /* canary words at the end of every stack */
#endif
#ifndef CONFIG_WATCHDOG
#define CONFIG_WATCHDOG   1     /* detects a stalled scheduler / heartbeat */
#endif

#endif
