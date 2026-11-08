/* console.c - the PlusOS text console: 64 columns x 42 rows of 8x8 glyphs
 * drawn straight into the frame buffer (ScrnBase, 64 bytes/row, 1 = black). */
#include "kernel.h"

extern const uint8_t plusos_font8x8[95 * 8];

static uint8_t *screen;
static int col, row;
static const int rows = CON_ROWS - 1;    /* last row is the status line */
static uint8_t inv;                 /* 0x00 normal, 0xFF inverted */
static int cursor_enabled, cursor_shown;
static uint32_t cursor_tick;

uint8_t *con_screen(void) { return screen; }

static uint8_t *cell(int c, int r) { return screen + r * 8 * 64 + c; }

static void draw_glyph(int c, int r, int ch)
{
    const uint8_t *g = plusos_font8x8 + ((ch >= 0x20 && ch < 0x7F) ? ch - 0x20 : '?' - 0x20) * 8;
    uint8_t *p = cell(c, r);
    for (int i = 0; i < 8; i++, p += 64)
        *p = g[i] ^ inv;
}

static void cursor_xor(void)
{
    if (col >= CON_COLS) return;
    uint8_t *p = cell(col, row) + 7 * 64;        /* underline on the bottom row */
    *p ^= 0xFE;
    cursor_shown ^= 1;
}

static void cursor_hide(void) { if (cursor_shown) cursor_xor(); }

void con_set_cursor(int on)
{
    cursor_enabled = on;
    if (on && !cursor_shown) { cursor_xor(); cursor_tick = LM_LONG(0x16A); }
    if (!on) cursor_hide();
}

void con_blink(void)
{
    if (!cursor_enabled) return;
    uint32_t t = LM_LONG(0x16A);                 /* Ticks */
    if (t - cursor_tick >= 30) { cursor_tick = t; cursor_xor(); }
}

static void scroll(void)
{
    memmove(screen, screen + 8 * 64, (rows - 1) * 8 * 64);
    memset(screen + (rows - 1) * 8 * 64, 0, 8 * 64);
}

static void newline(void)
{
    col = 0;
    if (++row >= rows) { row = rows - 1; scroll(); }
}

void con_putc(int c)
{
    int restore = cursor_shown;
    cursor_hide();
    switch (c) {
    case '\n': newline(); break;
    case '\r': col = 0; break;
    case '\b': if (col > 0) { col--; draw_glyph(col, row, ' '); } break;
    case '\t': do con_putc(' '); while (col & 7); break;
    case '\f': con_clear(); break;
    default:
        if (col >= CON_COLS) newline();
        draw_glyph(col, row, c & 0xFF);
        col++;
    }
    if (restore && cursor_enabled) cursor_xor();
}

void con_write(const char *s, size_t n) { while (n--) con_putc(*s++); }

void con_clear(void)
{
    memset(screen, 0, 342 * 64);
    col = row = 0;
    cursor_shown = 0;
}

void con_goto(int c, int r)
{
    cursor_hide();
    col = c < 0 ? 0 : c >= CON_COLS ? CON_COLS - 1 : c;
    row = r < 0 ? 0 : r >= rows ? rows - 1 : r;
}

void con_getpos(int *c, int *r) { if (c) *c = col; if (r) *r = row; }

/* Status line on the bottom row (white on black), outside the scrolling area. */
void con_status(const char *s)
{
    int saved_inv = inv;
    inv = 0xFF;
    for (int c = 0; c < CON_COLS; c++)
        draw_glyph(c, CON_ROWS - 1, *s ? *s++ : ' ');
    inv = (uint8_t)saved_inv;
}
void con_set_invert(int on) { inv = on ? 0xFF : 0x00; }

void con_init(void)
{
    screen = (uint8_t *)LM_LONG(0x824);          /* ScrnBase */
    inv = 0;
    cursor_enabled = cursor_shown = 0;
    con_clear();
}

/* libc back-end for the kernel */
void __plusos_con_putc(int c)                    { con_putc(c); }
void __plusos_con_write(const char *s, size_t n) { con_write(s, n); }
