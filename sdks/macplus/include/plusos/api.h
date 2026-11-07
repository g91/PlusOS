/* api.h - the PlusOS kernel service table.
 *
 * When the kernel runs a program it passes a pointer to this table. The app
 * runtime stores it in the global `plusos`, and the SDK's libc (printf,
 * getchar, malloc, ...) is built on it, so apps share the kernel's console,
 * keyboard queue and ROM heap. ROM traps (QuickDraw, Memory Manager, File
 * Manager, ...) are called directly through the ROM headers of your SDK:
 * <plusrom/plusrom.h> (Mac Plus SDK) or <serom/serom.h> (Mac SE SDK).
 *
 * The table only ever grows: new entries are appended and `version` bumped,
 * so old apps keep working on newer kernels.
 */
#ifndef PLUSOS_API_H
#define PLUSOS_API_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

#define PLUSOS_API_MAGIC    0x504F5341UL   /* 'POSA' */
#define PLUSOS_API_VERSION  1

/* flags: which ROM subsystems the kernel brought up at boot */
#define PLUSOS_HAVE_HEAP    0x0001   /* ROM application heap (NewPtr/NewHandle) */
#define PLUSOS_HAVE_EVENTS  0x0002   /* ROM event queue (keyboard/mouse events) */
#define PLUSOS_HAVE_FS      0x0004   /* ROM File Manager + boot volume mounted */
#define PLUSOS_HAVE_QD      0x0008   /* QuickDraw initialised, port on screen */

/* special key codes returned by getkey() (besides plain ASCII) - the
 * standard Mac character codes for these keys */
#define KEY_HOME       0x01
#define KEY_ENTER      0x03    /* keypad Enter */
#define KEY_END        0x04
#define KEY_HELP       0x05
#define KEY_PGUP       0x0B
#define KEY_PGDN       0x0C
#define KEY_FKEY       0x10    /* any function key */
#define KEY_ESC        0x1B    /* Esc, and keypad Clear */
#define KEY_DELETE     0x7F    /* forward delete */
#define KEY_BACKSPACE  0x08
#define KEY_TAB        0x09
#define KEY_RETURN     0x0D
#define KEY_CLEAR      0x1B    /* keypad Clear */
#define KEY_LEFT       0x1C
#define KEY_RIGHT      0x1D
#define KEY_UP         0x1E
#define KEY_DOWN       0x1F

typedef struct PlusOSAPI {
    uint32_t magic;                 /* PLUSOS_API_MAGIC */
    uint32_t version;               /* PLUSOS_API_VERSION */
    uint32_t flags;                 /* PLUSOS_HAVE_* */

    /* --- text console (64 x 42 characters, 8x8 font) --- */
    void (*con_putc)(int c);
    void (*con_write)(const char *s, size_t n);
    void (*con_clear)(void);
    void (*con_goto)(int col, int row);
    void (*con_getpos)(int *col, int *row);
    void (*con_invert)(int on);     /* white-on-black text */
    void (*con_cursor)(int on);     /* show/hide the text cursor */

    /* --- keyboard --- */
    int  (*key_get)(void);          /* wait for a key: ASCII or KEY_* */
    int  (*key_poll)(void);         /* next key or 0 if none waiting */
    int  (*key_line)(char *buf, int size);   /* line editor; returns length */
    uint16_t (*key_mods)(void);     /* modifier state (EventRecord style) */

    /* --- memory (ROM Memory Manager application heap) --- */
    void *(*mem_alloc)(uint32_t n);
    void  (*mem_free)(void *p);
    uint32_t (*mem_free_bytes)(void);

    /* --- time --- */
    uint32_t (*ticks)(void);        /* 60 Hz ticks since boot */
    uint32_t (*seconds)(void);      /* RTC: seconds since 1 Jan 1904 */

    /* --- storage on the boot drive (raw 512-byte blocks, partition relative) --- */
    int (*blk_read)(uint32_t block, uint32_t count, void *buf);
    int (*blk_write)(uint32_t block, uint32_t count, const void *buf);

    /* --- system --- */
    void (*reboot)(void);
    void (*shutdown)(void);
    int  (*run)(const char *name, int argc, char **argv);   /* run another program */

    /* --- environment --- */
    void    *qd;                    /* QDGlobals* (QuickDraw globals) */
    uint8_t *screen;                /* the live frame buffer: 512 x 342, 1 bpp, 1 = black */
    int16_t  screen_rowbytes;       /* 64 */
    int16_t  screen_width;          /* 512 */
    int16_t  screen_height;         /* 342 */
    int16_t  boot_vrefnum;          /* boot volume (File Manager), 0 if no FS */
    const uint8_t *font8x8;         /* 95 glyphs, ASCII 0x20..0x7E, 8 bytes each */

    /* --- double buffering (video driver) ---
     * Draw a whole frame into `screen_back`, then call present() to copy it to
     * the live screen in one pass. This removes flicker: the screen is never
     * seen half-drawn. present_rect(top, bottom) flips only those pixel rows. */
    uint8_t *screen_back;           /* off-screen buffer, same layout as screen */
    void (*present)(void);          /* flip the whole back buffer to the screen */
    void (*present_rect)(int top, int bottom);

    /* New members are APPENDED here so existing app binaries keep their layout. */
    void     (*sleep_ticks)(int ticks);  /* yield the CPU for ~ticks/60 s */
} PlusOSAPI;

/* In apps: the table passed by the kernel. In the kernel: its own table. */
extern const PlusOSAPI *plusos;

#ifdef __cplusplus
}
#endif
#endif
