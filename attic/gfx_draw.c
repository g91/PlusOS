/* gfx_draw.c - direct framebuffer drawing primitives for the PlusOS desktop.
 *
 * We draw straight into the 1-bpp screen (ScrnBase, 64 bytes/row, 512x342,
 * 1 = black) instead of using the ROM Window/Menu managers, which need a
 * System file's resources and fonts and hang bare-metal. These primitives plus
 * our 8x8 font are enough for a windowed desktop, and they work on the Plus and
 * the SE alike. The mouse cursor is the ROM's arrow (InitCursor/ShowCursor).
 */
#include "kernel.h"
#include "gfx.h"

extern const uint8_t plusos_font8x8[95 * 8];

#define ROWB 64
static uint8_t *fb(void) { return (uint8_t *)LM_LONG(0x824); }   /* ScrnBase */

void gd_pixel(int x, int y, int black)
{
    if ((unsigned)x >= 512 || (unsigned)y >= 342) return;
    uint8_t *p = fb() + y * ROWB + (x >> 3);
    uint8_t m = (uint8_t)(0x80 >> (x & 7));
    if (black) *p |= m; else *p &= (uint8_t)~m;
}

/* Fill [l,t)-(r,b) with a 2-row pattern (pat[y&1]); whole-byte fast path. */
void gd_fill_pat(int l, int t, int r, int b, uint8_t pat0, uint8_t pat1)
{
    if (l < 0) l = 0; if (t < 0) t = 0;
    if (r > 512) r = 512; if (b > 342) b = 342;
    if (l >= r || t >= b) return;
    for (int y = t; y < b; y++) {
        uint8_t pat = (y & 1) ? pat1 : pat0;
        uint8_t *row = fb() + y * ROWB;
        for (int x = l; x < r; x++) {
            uint8_t m = (uint8_t)(0x80 >> (x & 7));
            if (pat & m) row[x >> 3] |= m; else row[x >> 3] &= (uint8_t)~m;
        }
    }
}

void gd_fill(int l, int t, int r, int b, int black)
{
    gd_fill_pat(l, t, r, b, black ? 0xFF : 0x00, black ? 0xFF : 0x00);
}

void gd_gray(int l, int t, int r, int b)      /* 50% desktop gray */
{
    gd_fill_pat(l, t, r, b, 0xAA, 0x55);
}

void gd_frame(int l, int t, int r, int b, int thick)
{
    for (int i = 0; i < thick; i++) {
        for (int x = l; x < r; x++) { gd_pixel(x, t + i, 1); gd_pixel(x, b - 1 - i, 1); }
        for (int y = t; y < b; y++) { gd_pixel(l + i, y, 1); gd_pixel(r - 1 - i, y, 1); }
    }
}

void gd_hline(int l, int r, int y, int black) { for (int x = l; x < r; x++) gd_pixel(x, y, black); }
void gd_vline(int x, int t, int b, int black) { for (int y = t; y < b; y++) gd_pixel(x, y, black); }

/* Invert a 1px rectangle outline (for XOR drag feedback). */
void gd_xor_frame(int l, int t, int r, int b)
{
    for (int x = l; x < r; x++) { gd_pixel_xor(x, t); gd_pixel_xor(x, b - 1); }
    for (int y = t; y < b; y++) { gd_pixel_xor(l, y); gd_pixel_xor(r - 1, y); }
}

void gd_pixel_xor(int x, int y)
{
    if ((unsigned)x >= 512 || (unsigned)y >= 342) return;
    uint8_t *p = fb() + y * ROWB + (x >> 3);
    *p ^= (uint8_t)(0x80 >> (x & 7));
}

/* One 8x8 glyph at pixel (x,y). black=1 draws ink on white; invert swaps. */
void gd_char(int x, int y, int ch, int invert)
{
    const uint8_t *g = plusos_font8x8 + ((ch >= 0x20 && ch < 0x7F) ? ch - 0x20 : 0) * 8;
    for (int row = 0; row < 8; row++) {
        uint8_t bits = g[row] ^ (invert ? 0xFF : 0x00);
        for (int col = 0; col < 8; col++)
            if (bits & (0x80 >> col)) gd_pixel(x + col, y + row, 1);
            else gd_pixel(x + col, y + row, 0);
    }
}

int gd_text(int x, int y, const char *s, int invert)
{
    while (*s) { gd_char(x, y, (uint8_t)*s++, invert); x += 8; }
    return x;
}

int gd_text_w(const char *s) { int n = 0; while (s[n]) n++; return n * 8; }
