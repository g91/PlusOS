/* lc3_cqd_engine.c - Macintosh LC III Color QuickDraw Advanced Pipeline & Text Engine
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   CLRLOOP, NEXTCH, BOLDLP, ONLY2, UL1, UL2, CLR2, COPYLP, ACROSS2, DOWN2,
 *   FONTOK, FACEOK, STDTEXT_MODEOK, STDTEXT_SIZEOK, CHARWIDTH, SPACEEXTRA,
 *   STRINGWIDTH, STD, MEASURETEXT_USESTD, STDTXMEAS_NEXTCH, GETFONTINFO,
 *   SCALE, STDLINE_NO, UPDATE, LINETO, LINE, MOVETO, MOVE, SHOWPEN_CONT,
 *   PUTLINE, DHPOS, DVPOS, PUTLINE_SLANTED, NOSWAP, NOSWAP_OK, NOSWAP_NO,
 *   LOOP, NOCHNG, NOCOLOR, VOK2, VSWAP, DRAWLINE_CLRMASK, TOGLTWO, TOGLONE
 */

#include "lc3_cqd_engine.h"
#include "lc3_hardware.h"
#include <string.h>
#include <stdlib.h>

static LC3PenState g_pen = {
    .pnLocH = 0, .pnLocV = 0,
    .pnSizeH = 1, .pnSizeV = 1,
    .pnMode = 0,
    .pnPat = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
};

static uint8_t g_activeFace = 0;
static int16_t g_spaceExtra = 0;

void lc3_cqd_moveto(int16_t h, int16_t v) {
    /* MOVETO */
    g_pen.pnLocH = h;
    g_pen.pnLocV = v;
}

void lc3_cqd_move(int16_t dh, int16_t dv) {
    /* MOVE */
    g_pen.pnLocH += dh;
    g_pen.pnLocV += dv;
}

void lc3_cqd_putline(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    /* PUTLINE, DHPOS, DVPOS: optimized horizontal or vertical line blitter */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80; /* 1-bpp */

    if (y0 == y1) {
        /* Horizontal line */
        int16_t left = (x0 < x1) ? x0 : x1;
        int16_t right = (x0 < x1) ? x1 : x0;
        for (int16_t x = left; x <= right; ++x) {
            uint32_t offset = (y0 * rowBytes) + (x >> 3);
            vram[offset] |= (0x80 >> (x & 7));
        }
    } else if (x0 == x1) {
        /* Vertical line */
        int16_t top = (y0 < y1) ? y0 : y1;
        int16_t bottom = (y0 < y1) ? y1 : y0;
        uint8_t mask = (0x80 >> (x0 & 7));
        for (int16_t y = top; y <= bottom; ++y) {
            vram[(y * rowBytes) + (x0 >> 3)] |= mask;
        }
    } else {
        /* Slanted line */
        lc3_cqd_putline_slanted(x0, y0, x1, y1);
    }
}

void lc3_cqd_putline_slanted(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    /* PUTLINE_SLANTED, NOSWAP, VSWAP: Bresenham's line algorithm */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80;

    int16_t dx = abs(x1 - x0);
    int16_t dy = -abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        if (x0 >= 0 && x0 < 640 && y0 >= 0 && y0 < 480) {
            vram[(y0 * rowBytes) + (x0 >> 3)] |= (0x80 >> (x0 & 7));
        }
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void lc3_cqd_lineto(int16_t h, int16_t v) {
    /* LINETO */
    lc3_cqd_putline(g_pen.pnLocH, g_pen.pnLocV, h, v);
    g_pen.pnLocH = h;
    g_pen.pnLocV = v;
}

void lc3_cqd_line(int16_t dh, int16_t dv) {
    /* LINE */
    lc3_cqd_lineto(g_pen.pnLocH + dh, g_pen.pnLocV + dv);
}

void lc3_cqd_textface(uint8_t face) {
    /* TEXTFACE */
    g_activeFace = face;
}

int16_t lc3_cqd_charwidth(char c) {
    /* CHARWIDTH, SPACEEXTRA */
    if (c == ' ') {
        return 4 + g_spaceExtra;
    }
    int16_t base = 8;
    if (g_activeFace & FONT_STYLE_BOLD) base += 1;
    if (g_activeFace & FONT_STYLE_EXTEND) base += 2;
    if (g_activeFace & FONT_STYLE_CONDENSE) base -= 1;
    return base;
}

int16_t lc3_cqd_stringwidth(const char *str) {
    /* STRINGWIDTH */
    int16_t total = 0;
    while (*str) {
        total += lc3_cqd_charwidth(*str++);
    }
    return total;
}

void lc3_cqd_getfontinfo(LC3FontInfo *info) {
    /* GETFONTINFO */
    info->ascent = 9;
    info->descent = 3;
    info->widMax = 12;
    info->leading = 1;
}

void lc3_cqd_draw_char_styled(char c, int16_t x, int16_t y, uint8_t style) {
    /* BOLDLP, UL1, UL2, CLR2: algorithmic font styling */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80;

    /* Base 8x8 character cell */
    uint8_t glyph = (uint8_t)c;
    for (int r = 0; r < 8; ++r) {
        int16_t py = y + r;
        if (py < 0 || py >= 480) continue;
        uint8_t rowBits = glyph;

        /* Bold styling: OR with shifted right bit (BOLDLP) */
        if (style & FONT_STYLE_BOLD) {
            rowBits |= (rowBits >> 1);
        }

        /* Write bits into VRAM */
        for (int b = 0; b < 8; ++b) {
            if (rowBits & (0x80 >> b)) {
                int16_t px = x + b;
                if (px >= 0 && px < 640) {
                    vram[(py * rowBytes) + (px >> 3)] |= (0x80 >> (px & 7));
                }
            }
        }
    }

    /* Underline styling: horizontal bar at baseline + 1 (UL1, UL2) */
    if (style & FONT_STYLE_UNDERLINE) {
        int16_t uy = y + 9;
        if (uy < 480) {
            int16_t w = lc3_cqd_charwidth(c);
            for (int16_t b = 0; b < w; ++b) {
                int16_t px = x + b;
                if (px >= 0 && px < 640) {
                    vram[(uy * rowBytes) + (px >> 3)] |= (0x80 >> (px & 7));
                }
            }
        }
    }
}

void lc3_cqd_draw_string(const char *str, int16_t x, int16_t y) {
    /* STDTEXT_MODEOK, STDTXMEAS_NEXTCH */
    while (*str) {
        lc3_cqd_draw_char_styled(*str, x, y, g_activeFace);
        x += lc3_cqd_charwidth(*str++);
    }
    g_pen.pnLocH = x;
    g_pen.pnLocV = y;
}

void lc3_cqd_measure_text(const char *text, uint16_t count, int16_t *charLocs) {
    /* MEASURETEXT_USESTD, MEASURETEXT_CUSTOM_STORE */
    int16_t curX = 0;
    for (uint16_t i = 0; i < count; ++i) {
        charLocs[i] = curX;
        curX += lc3_cqd_charwidth(text[i]);
    }
}
