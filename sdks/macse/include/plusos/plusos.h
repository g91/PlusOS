/* plusos.h - PlusOS SDK umbrella header.
 *
 *   #include <plusos/plusos.h>     PlusOS services + libc
 *   #include <plusrom/plusrom.h>   the whole Mac Plus ROM (Mac Plus SDK), or
 *   #include <serom/serom.h>       the whole Mac SE ROM (Mac SE SDK):
 *                                  Toolbox + OS traps, low memory, hardware
 *
 * Program entry point:  int main(int argc, char **argv)
 */
#ifndef PLUSOS_PLUSOS_H
#define PLUSOS_PLUSOS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <plusos/api.h>
#include <plusos/exe.h>
#include <plusos/theme.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Convenience wrappers around the service table (apps only - the kernel
 * implements these names itself and defines PLUSOS_IN_KERNEL). */
#ifndef PLUSOS_IN_KERNEL
static inline void     con_clear(void)               { plusos->con_clear(); }
static inline void     con_goto(int col, int row)    { plusos->con_goto(col, row); }
static inline void     con_invert(int on)            { plusos->con_invert(on); }
static inline int      key_get(void)                 { return plusos->key_get(); }
static inline int      key_poll(void)                { return plusos->key_poll(); }
static inline uint32_t ticks(void)                   { return plusos->ticks(); }
static inline void     sleep_ticks(uint32_t n)       { uint32_t end = plusos->ticks() + n; while (plusos->ticks() < end) {} }
#endif
static inline int      plusos_has(uint32_t flag)     { return (plusos->flags & flag) != 0; }

/* Direct pixel access on 1bpp framebuffers using dynamic screen stride. */
static inline void pixel_set(int x, int y)
{
    int rb = plusos->screen_rowbytes ? plusos->screen_rowbytes : 64;
    plusos->screen[y * rb + (x >> 3)] |= (uint8_t)(0x80 >> (x & 7));
}
static inline void pixel_clear(int x, int y)
{
    int rb = plusos->screen_rowbytes ? plusos->screen_rowbytes : 64;
    plusos->screen[y * rb + (x >> 3)] &= (uint8_t)~(0x80 >> (x & 7));
}

#ifdef __cplusplus
}
#endif
#endif
