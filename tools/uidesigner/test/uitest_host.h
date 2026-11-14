/* uitest_host.h - a fake PlusOS kernel for headless PlusUI tests on the bare 68k
 * harness (emulator/tools/run68k.mjs): heap, ticks, mouse globals, frame dumps and
 * helpers that feed clicks and keys to an attached form. Include it once. */
#ifndef UITEST_HOST_H
#define UITEST_HOST_H
#include <plusos/plusos.h>
#include <plusos/gfx.h>
#include <plusos/ui.h>
#include <string.h>
#include <stdio.h>

static void putch(int c) { *(volatile unsigned char *)0x00FF0000 = (unsigned char)c; }
static void puts_(const char *s) { while (*s) putch(*s++); }
static void putnum(long v) { char b[16]; int n = 0; if (v < 0) { putch('-'); v = -v; } do { b[n++] = (char)('0' + v % 10); v /= 10; } while (v); while (n) putch(b[--n]); }

/* ---- a fake kernel ---- */
extern const uint8_t plusos_font8x8[];
static uint8_t heap[256 * 1024];
static uint32_t heap_top;
static void *k_alloc(uint32_t n) { n = (n + 3) & ~3u; if (heap_top + n > sizeof heap) return 0; void *p = heap + heap_top; heap_top += n; return p; }
static void k_free(void *p) { (void)p; }
void *__plusos_mem_alloc(uint32_t n) { return k_alloc(n); }
void  __plusos_mem_free(void *p) { (void)p; }
void  __plusos_con_putc(int c) { putch(c); }
void  __plusos_con_write(const char *s, size_t n) { while (n--) putch(*s++); }
int   __plusos_key_get(void) { return 0; }
int   __plusos_key_line(char *b, int n) { (void)n; b[0] = 0; return 0; }
static uint32_t tick;
static uint32_t k_ticks(void) { return tick; }
static uint16_t k_mods(void) { return 0; }
static uint8_t fakescreen[64 * 342];

static GSurface *dump_surf;
static const char *dump_name;
static int release_at;              /* sleep call on which the mouse button is released */
static int sleeps;
static void dump(const char *name)
{
    GSurface *s = dump_surf;
    puts_("FRAME "); puts_(name); putch(' '); putnum(s->w); putch(' '); putnum(s->h); putch(' '); putnum(s->rowbytes); putch('\n');
    for (int y = 0; y < s->h; y++) {
        for (int x = 0; x < s->rowbytes; x++) {
            uint8_t b = s->bits[y * s->rowbytes + x];
            putch("0123456789abcdef"[b >> 4]); putch("0123456789abcdef"[b & 15]);
        }
        putch('\n');
    }
}
static void mouse_set(int x, int y, int down)
{
    *(volatile int16_t *)0x832 = (int16_t)x; *(volatile int16_t *)0x830 = (int16_t)y;
    *(volatile uint8_t *)0x172 = down ? 0x00 : 0x80;
}
static int move_x = -1, move_y;
static void k_sleep(int t)
{
    tick += (uint32_t)t;
    if (release_at && ++sleeps == release_at) {
        if (move_x >= 0) { mouse_set(move_x, move_y, 1); move_x = -1; release_at = sleeps + 3; return; }
        if (dump_name) dump(dump_name);
        *(volatile uint8_t *)0x172 = 0x80;
        release_at = 0;
    }
}
static PlusOSAPI api;
const PlusOSAPI *plusos = &api;

/* point the fake kernel at the harness; TRAP #1 (syscalls) just returns 0 */
static void host_init(void)
{
    static const uint16_t trap1[] = { 0x7000, 0x4E73 };        /* moveq #0,d0 ; rte */
    *(volatile uint32_t *)0x84 = (uint32_t)trap1;
    api.font8x8 = plusos_font8x8; api.mem_alloc = k_alloc; api.mem_free = k_free;
    api.ticks = k_ticks; api.key_mods = k_mods; api.sleep_ticks = k_sleep;
    api.screen = api.screen_back = fakescreen; api.screen_rowbytes = 64; api.screen_width = 512; api.screen_height = 342;
    mouse_set(0, 0, 0);
}

static int failures;
#define CHECK(c) do { if (!(c)) { puts_("FAIL line "); putnum(__LINE__); puts_(": " #c "\n"); failures++; } } while (0)

/* click at client point; the press is released straight away */
static void click(UIForm *f, int x, int y)
{
    GEvent e = { GEV_MOUSEDOWN, x, y + f->top, 0 };
    mouse_set(x, y + f->top, 0);
    ui_form_dispatch(f, &e);
    ui_form_draw(f);
}
/* press at a surface point and hold; after `hold` sleeps (optionally moving first) dump and release */
static void press_hold(UIForm *f, int sx, int sy, int hold, const char *name)
{
    GEvent e = { GEV_MOUSEDOWN, sx, sy, 0 };
    mouse_set(sx, sy, 1);
    sleeps = 0; release_at = hold; dump_name = name;
    ui_form_dispatch(f, &e);
    release_at = 0; dump_name = 0;
    ui_form_draw(f);
}
static void key(UIForm *f, int k) { GEvent e = { GEV_KEY, 0, 0, k }; ui_form_dispatch(f, &e); ui_form_draw(f); }
static void type(UIForm *f, const char *s) { while (*s) key(f, *s++); }

#endif
