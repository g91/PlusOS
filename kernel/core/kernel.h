/* kernel.h - PlusOS kernel internals. */
#ifndef PLUSOS_KERNEL_H
#define PLUSOS_KERNEL_H

#define PLUSOS_IN_KERNEL 1
#include "kconfig.h"
#include "kmodule.h"
/* The kernel runs on the Mac Plus and the Mac SE. It is built with the Mac
 * Plus SDK (every Plus trap is also in the SE ROM) and also reads the Mac SE
 * SDK's call catalog for the `trap` / `rom` shell commands. */
#include <plusrom/plusrom.h>
#include <serom/traps.h>
#include <serom/catalog.h>
#include <plusos/plusos.h>

#define PLUSOS_VERSION   "2.0"
#define CON_COLS         64
#define CON_ROWS         42
#define KERNEL_STACK     0x10000        /* must match crt0 (PLUSOS_STACK_SIZE) */
#define APPS_BLOCK       61000          /* raw app store, see config.mk */
#define USTACK_SIZE      16384          /* per-process user stack (ring 2) */
#define MAX_ARGS         16

/* Interrupt control (supervisor only). int_disable raises the mask to 7 and
 * returns the old SR; int_restore puts it back; int_enable lowers it to 0. */
static inline uint16_t int_disable(void)
{
    uint16_t sr;
    __asm__ volatile("move.w %%sr,%0\n\tori.w #0x0700,%%sr" : "=d"(sr) : : "cc", "memory");
    return sr;
}
static inline void int_restore(uint16_t sr)
{
    __asm__ volatile("move.w %0,%%sr" : : "d"(sr) : "cc", "memory");
}
static inline void int_enable(void)
{
    __asm__ volatile("andi.w #0xF8FF,%%sr" : : : "cc", "memory");
}

/* ---- globals (main.c) ---- */
extern uint32_t   __plusos_load_addr;   /* where the boot block put us (crt0) */
extern QDGlobals  qd;
extern GrafPort   g_port;               /* QuickDraw port on the screen */
extern PlusOSAPI  g_api;                /* the service table handed to apps */
extern uint32_t   g_flags;              /* PLUSOS_HAVE_* */
extern int16_t    g_boot_drive;         /* drive number */
extern int16_t    g_boot_refnum;        /* its driver */
extern int16_t    g_boot_vref;          /* mounted volume, 0 if no file system */
extern char       g_volname[32];

/* ---- machine ---- */
static inline int machine_is_se(void) { return plusrom_machine() == PLUSROM_MACHINE_SE; }

/* ---- console.c: 64x42 text on the 512x342 frame buffer ---- */
void con_init(void);
void con_putc(int c);
void con_write(const char *s, size_t n);
void con_clear(void);
void con_goto(int col, int row);
void con_getpos(int *col, int *row);
void con_set_invert(int on);
void con_set_cursor(int on);
void con_blink(void);                   /* call while idle: blinks the cursor */
void con_status(const char *s);         /* bottom-row status line */
uint8_t *con_screen(void);

/* ---- video.c: off-screen back buffer + page-flip present (anti-flicker) ---- */
void      video_init(void);
uint8_t  *video_front(void);     /* the live screen (ScrnBase) */
uint8_t  *video_back(void);      /* off-screen drawing buffer */
void      video_present(void);                     /* flip the whole frame */
void      video_present_rect(int top, int bottom); /* flip rows [top, bottom) */

/* ---- kbd.c ---- */
void     kbd_init(int use_events);
int      kbd_get(void);
int      kbd_get_click(void);           /* key, or KEY_CLICK for a mouse click */
#define  KEY_CLICK 0x100
int      kbd_poll(void);
int      kbd_line(char *buf, int size);
uint16_t kbd_mods(void);
int      kbd_events(void);
int      kbd_status(char *buf, int size);
int      kbd_raw_next(void);            /* next raw key transition, -1 if none */
int      keymap_translate(int code, int modword);   /* keymap.c */
int      kbd_history_count(void);
const char *kbd_history_get(int index);
void     kbd_history_add(const char *line);
void     kbd_history_clear(void);
int      kbd_tab_complete(const char *base_buf, int base_len, char *buf, int size,
                          int tab_idx, char *status_out, int status_sz);

/* ---- modules.c: the module table (see kmodule.h) ---- */
void modules_register(void);

/* ---- rominit.c: bring up the ROM's subsystems ---- */
void        rom_init(void);
const char *cpu_name(void);
uint32_t    ram_bytes(void);

/* ---- api.c ---- */
void api_init(void);
int  blk_read(uint32_t block, uint32_t count, void *buf);
int  blk_write(uint32_t block, uint32_t count, const void *buf);
void *kmalloc(uint32_t n);
void  kfree(void *p);

/* ---- exec.c: load and run .pex programs ---- */
int  exec_run(const char *name, int argc, char **argv);
int  exec_spawn(const char *name, int argc, char **argv);   /* ring-2 process; Stage 2 */
void cache_flush(void);
const char *exec_error(int code);

/* ---- kbd.c blocking wait (used by the syscall gate) ---- */
int  kbd_wait(void);                    /* block the current process until a key */

/* ---- sys.c ---- */
__attribute__((noreturn)) void sys_reboot(void);
__attribute__((noreturn)) void sys_shutdown(void);

/* ---- chainboot.c: start another OS from a chosen SCSI ID (Mac SE) ---- */
int  chainboot(int scsi_id);            /* set default + restart; no return on success */
int  chainboot_set_default(int scsi_id);
int  chainboot_get_default(void);       /* current default SCSI ID, or -1 */
void chainboot_each_drive(void (*fn)(int refnum, int drive));

/* ---- config.c: persistent settings (PLUSOS_CFG_* keys in plusos/syscall.h) ---- */
int  cfg_get(int key);
int  cfg_set(int key, int value);      /* saves to the boot volume; 0 or <0 */

/* ---- registry.c: system-wide key=value settings, saved to "PlusOS Registry" ---- */
int  reg_get(const char *key, char *out, int size);     /* length, or -1 if absent */
int  reg_set(const char *key, const char *value);       /* empty value deletes */
int  reg_enum(int index, char *key, int ksize, char *value, int vsize);  /* 0 or -1 */

/* ---- sound.c: Compact Mac hardware sound synthesizer & volume control ---- */
struct plusos_sound_args;
#if CONFIG_SOUND
void sound_init(void);
void sound_vbl_tick(void);
void sound_beep(int duration_ticks);
void sound_tone(int freq_hz, int duration_ticks);
void sound_play_notes(const uint16_t *notes, int count);
int  sound_get_volume(void);
void sound_set_volume(int vol);
long sound_syscall(struct plusos_sound_args *a);
#else   /* CONFIG_SOUND=0: no sound driver; everything is a no-op */
static inline void sound_init(void) {}
static inline void sound_vbl_tick(void) {}
static inline void sound_beep(int t) { (void)t; }
static inline void sound_tone(int f, int t) { (void)f; (void)t; }
static inline void sound_play_notes(const uint16_t *n, int c) { (void)n; (void)c; }
static inline int  sound_get_volume(void) { return 0; }
static inline void sound_set_volume(int v) { (void)v; }
static inline long sound_syscall(struct plusos_sound_args *a) { (void)a; return -1; }
#endif

/* ---- scrap.c: system-wide clipboard ("Clipboard File") ---- */
struct plusos_scrap_args;
#if CONFIG_SCRAP
long scrap_syscall(struct plusos_scrap_args *a);
#else
static inline long scrap_syscall(struct plusos_scrap_args *a) { (void)a; return -1; }
#endif

/* ---- macloader.c: Classic Macintosh 'APPL' ('CODE' resource) loader ---- */
#if CONFIG_MACLOADER
int  mac_try_run(const char *name, int argc, char **argv);
#else
static inline int mac_try_run(const char *n, int c, char **v) { (void)n; (void)c; (void)v; return -1001; /* E_NOTFOUND */ }
#endif

/* ---- crash.c: unrecoverable kernel faults ---- */
__attribute__((noreturn)) void kernel_panic(const char *what, const char *who);

/* ---- crash.c / exc.S: exceptions and system errors return to the shell ---- */
void exc_install(void);

/* ---- shell.c / menu.c / gfx.c ---- */
__attribute__((noreturn)) void shell_main(void);
__attribute__((noreturn)) void menu_main(void);
void gfx_demo(void);
void gfx_mouse(void);
void gfx_paint(void);

#endif
