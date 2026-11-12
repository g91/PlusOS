/* console.c - PlusOS multi-depth dynamic text console (8x8 glyphs).
 *
 * Adapts automatically to the active framebuffer geometry discovered by
 * video_probe_geometry() (512x342 on Compact Macs, 640x480 / 832x624 /
 * 1152x870 on Modular/LC/Quadra Macs) and renders cleanly across 1, 2, 4,
 * 8 (CLUT), 16 (555 direct), and 32 (xRGB direct) bits per pixel.
 */
#include "kernel.h"

extern const uint8_t plusos_font8x8[95 * 8];

static uint8_t *screen;
static int col, row;
static int max_cols = CON_COLS;
static int max_rows = CON_ROWS;
static int scroll_rows = CON_ROWS - 1;  /* bottom row reserved for status line */
static uint8_t inv;                     /* 0x00 normal (black on white), 0xFF inverted */
static int cursor_enabled, cursor_shown;
static uint32_t cursor_tick;
static int stride = 64;                 /* bytes per scanline */
static int screen_rows = 342;           /* pixel height */
static int depth = 1;                   /* bpp: 1, 2, 4, 8, 16, 32 */

uint8_t *con_screen(void) { return screen; }
int      con_cols(void)   { return max_cols; }
int      con_rows(void)   { return max_rows; }

static inline uint8_t *cell(int c, int r)
{
    return screen + (uint32_t)r * 8U * (uint32_t)stride + (uint32_t)(c * depth);
}

static void render_glyph_row(uint8_t *p, uint8_t bits)
{
    switch (depth) {
    case 1:
        *p = bits;
        break;
    case 2: {
        for (int b = 0; b < 2; b++) {
            uint8_t out = 0;
            for (int k = 0; k < 4; k++) {
                uint8_t px = (bits & (0x80 >> (b * 4 + k))) ? 3U : 0U;
                out = (uint8_t)((out << 2) | px);
            }
            p[b] = out;
        }
        break;
    }
    case 4: {
        for (int b = 0; b < 4; b++) {
            uint8_t hi = (bits & (0x80 >> (b * 2)))     ? 0xF0U : 0x00U;
            uint8_t lo = (bits & (0x80 >> (b * 2 + 1))) ? 0x0FU : 0x00U;
            p[b] = (uint8_t)(hi | lo);
        }
        break;
    }
    case 8: {
        for (int k = 0; k < 8; k++)
            p[k] = (bits & (0x80 >> k)) ? 0xFFU : 0x00U;
        break;
    }
    case 16: {
        uint16_t *w = (uint16_t *)p;
        for (int k = 0; k < 8; k++)
            w[k] = (bits & (0x80 >> k)) ? 0x0000U : 0x7FFFU;
        break;
    }
    case 32: {
        uint32_t *dw = (uint32_t *)p;
        for (int k = 0; k < 8; k++)
            dw[k] = (bits & (0x80 >> k)) ? 0x00000000UL : 0x00FFFFFFUL;
        break;
    }
    default:
        *p = bits;
        break;
    }
}

static void fill_white(uint8_t *dst, uint32_t bytes)
{
    if (depth == 16) {
        uint16_t *w = (uint16_t *)dst;
        for (uint32_t i = 0; i < bytes / 2; i++) w[i] = 0x7FFFU;
    } else if (depth == 32) {
        uint32_t *dw = (uint32_t *)dst;
        for (uint32_t i = 0; i < bytes / 4; i++) dw[i] = 0x00FFFFFFUL;
    } else {
        memset(dst, 0, bytes);
    }
}

static void draw_glyph(int c, int r, int ch)
{
    if (c < 0 || c >= max_cols || r < 0 || r >= max_rows) return;
    const uint8_t *g = plusos_font8x8 + ((ch >= 0x20 && ch < 0x7F) ? ch - 0x20 : '?' - 0x20) * 8;
    uint8_t *p = cell(c, r);
    for (int i = 0; i < 8; i++, p += stride)
        render_glyph_row(p, (uint8_t)(g[i] ^ inv));
}

static void cursor_xor(void)
{
    if (col >= max_cols) return;
    uint8_t *p = cell(col, row) + 7 * stride;
    if (depth == 1) {
        *p ^= 0xFE;
    } else if (depth == 16) {
        uint16_t *w = (uint16_t *)p;
        for (int k = 0; k < 7; k++) w[k] ^= 0x7FFFU;
    } else if (depth == 32) {
        uint32_t *dw = (uint32_t *)p;
        for (int k = 0; k < 7; k++) dw[k] ^= 0x00FFFFFFUL;
    } else {
        for (int i = 0; i < depth; i++) p[i] ^= 0xFFU;
    }
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
    uint32_t t = LM_LONG(0x16A);
    if (t - cursor_tick >= 30) { cursor_tick = t; cursor_xor(); }
}

static void scroll(void)
{
    uint32_t band = 8U * (uint32_t)stride;
    memmove(screen, screen + band, (uint32_t)(scroll_rows - 1) * band);
    fill_white(screen + (uint32_t)(scroll_rows - 1) * band, band);
}

static void newline(void)
{
    col = 0;
    if (++row >= scroll_rows) { row = scroll_rows - 1; scroll(); }
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
        if (col >= max_cols) newline();
        draw_glyph(col, row, c & 0xFF);
        col++;
    }
    if (restore && cursor_enabled) cursor_xor();
}

void con_write(const char *s, size_t n) { while (n--) con_putc(*s++); }

void con_clear(void)
{
    fill_white(screen, (uint32_t)screen_rows * (uint32_t)stride);
    col = row = 0;
    cursor_shown = 0;
}

void con_goto(int c, int r)
{
    cursor_hide();
    col = c < 0 ? 0 : c >= max_cols ? max_cols - 1 : c;
    row = r < 0 ? 0 : r >= scroll_rows ? scroll_rows - 1 : r;
}

void con_getpos(int *c, int *r) { if (c) *c = col; if (r) *r = row; }

void con_status(const char *s)
{
    int saved_inv = inv;
    inv = 0xFF;
    for (int c = 0; c < max_cols; c++)
        draw_glyph(c, max_rows - 1, *s ? *s++ : ' ');
    inv = (uint8_t)saved_inv;
}

void con_set_invert(int on) { inv = on ? 0xFF : 0x00; }

void con_init(void)
{
    int w = 512;
    video_probe_geometry(&screen, &stride, &w, &screen_rows, &depth);
    max_cols = w / 8;
    if (max_cols < 40) max_cols = CON_COLS;
    max_rows = screen_rows / 8;
    if (max_rows < 20) max_rows = CON_ROWS;
    scroll_rows = max_rows - 1;
    inv = 0;
    cursor_enabled = cursor_shown = 0;
    con_clear();
}

void __plusos_con_putc(int c)                    { con_putc(c); }
void __plusos_con_write(const char *s, size_t n) { con_write(s, n); }
