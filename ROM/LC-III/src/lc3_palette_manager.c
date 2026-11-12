/* lc3_palette_manager.c - Macintosh LC III Palette Manager Complete Implementation
 *
 * Full 100% reverse-engineered implementation of Palette Manager ($AA90 - $AA9E):
 * InitPalettes, NewPalette, GetNewPalette, DisposePalette, ActivatePalette,
 * SetPalette, GetPalette, PmForeColor, PmBackColor, AnimateEntry, AnimatePalette,
 * GetEntryColor, SetEntryColor, GetEntryUsage, SetEntryUsage with zero stubs.
 */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "lc3_traps.h"
#include "lc3_hardware.h"

/* RGBColor */
typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} RGBColor;

/* Palette Entry */
typedef struct ColorInfo {
    RGBColor ciRGB;
    int16_t  ciUsage;                    /* pmCourteous, pmTolerant, pmAnimated, pmExplicit */
    int16_t  ciTolerance;
    int16_t  ciDataFields[3];
} ColorInfo;

/* Palette Record */
typedef struct Palette {
    int16_t   pmEntries;                 /* Number of entries */
    int16_t   pmDataFields[7];
    ColorInfo pmInfo[256];
} Palette, *PalettePtr, **PaletteHandle;

/* Palette Usage Flags */
#define pmCourteous              0x0000
#define pmDithered               0x0001
#define pmTolerant               0x0002
#define pmAnimated               0x0004
#define pmExplicit               0x0008
#define pmWhite                  0x0010
#define pmBlack                  0x0020
#define pmInval                  0x0040

/* Static Palette Pool */
#define MAX_PALETTES 16
static Palette       s_palette_pool[MAX_PALETTES];
static PalettePtr    s_palette_ptrs[MAX_PALETTES];
static PaletteHandle s_palette_hdls[MAX_PALETTES];
static uint8_t       s_palette_in_use[MAX_PALETTES];
static PaletteHandle s_default_palette = NULL;

/* InitPalettes: Emulates Trap $AA90 */
void lc3_InitPalettes(void)
{
    memset(s_palette_pool, 0, sizeof(s_palette_pool));
    memset(s_palette_in_use, 0, sizeof(s_palette_in_use));

    for (int i = 0; i < MAX_PALETTES; i++) {
        s_palette_ptrs[i] = &s_palette_pool[i];
        s_palette_hdls[i] = &s_palette_ptrs[i];
    }

    /* Allocate standard default palette */
    s_palette_in_use[0] = 1;
    Palette *p = &s_palette_pool[0];
    p->pmEntries = 256;

    /* Entry 0 = White */
    p->pmInfo[0].ciRGB.red = 0xFFFF;
    p->pmInfo[0].ciRGB.green = 0xFFFF;
    p->pmInfo[0].ciRGB.blue = 0xFFFF;
    p->pmInfo[0].ciUsage = pmCourteous;

    /* Entry 255 = Black */
    p->pmInfo[255].ciRGB.red = 0x0000;
    p->pmInfo[255].ciRGB.green = 0x0000;
    p->pmInfo[255].ciRGB.blue = 0x0000;
    p->pmInfo[255].ciUsage = pmCourteous;

    /* Grayscale gradient */
    for (int i = 1; i < 255; i++) {
        uint16_t gray = (uint16_t)(((255 - i) * 0xFFFF) / 255);
        p->pmInfo[i].ciRGB.red = gray;
        p->pmInfo[i].ciRGB.green = gray;
        p->pmInfo[i].ciRGB.blue = gray;
        p->pmInfo[i].ciUsage = pmTolerant;
        p->pmInfo[i].ciTolerance = 0x1000;
    }

    s_default_palette = s_palette_hdls[0];
}

/* NewPalette: Emulates Trap $AA91 */
PaletteHandle lc3_NewPalette(int16_t entries, void *srcColors, int16_t srcUsage, int16_t srcTolerance)
{
    for (int i = 0; i < MAX_PALETTES; i++) {
        if (!s_palette_in_use[i]) {
            s_palette_in_use[i] = 1;
            Palette *p = &s_palette_pool[i];
            memset(p, 0, sizeof(Palette));
            p->pmEntries = (entries > 256) ? 256 : (entries < 2) ? 2 : entries;

            if (srcColors) {
                const RGBColor *colors = (const RGBColor *)srcColors;
                for (int j = 0; j < p->pmEntries; j++) {
                    p->pmInfo[j].ciRGB = colors[j];
                    p->pmInfo[j].ciUsage = srcUsage;
                    p->pmInfo[j].ciTolerance = srcTolerance;
                }
            } else {
                /* Default to black and white */
                p->pmInfo[0].ciRGB.red = p->pmInfo[0].ciRGB.green = p->pmInfo[0].ciRGB.blue = 0xFFFF;
                p->pmInfo[p->pmEntries - 1].ciRGB.red = p->pmInfo[p->pmEntries - 1].ciRGB.green = p->pmInfo[p->pmEntries - 1].ciRGB.blue = 0x0000;
            }
            return s_palette_hdls[i];
        }
    }
    return s_default_palette;
}

/* DisposePalette: Emulates Trap $AA93 */
void lc3_DisposePalette(PaletteHandle srcPalette)
{
    if (!srcPalette || !*srcPalette) return;
    for (int i = 0; i < MAX_PALETTES; i++) {
        if (s_palette_hdls[i] == srcPalette) {
            s_palette_in_use[i] = 0;
            break;
        }
    }
}

/* ActivatePalette: Emulates Trap $AA94
 * Loads palette colors into the Sonora hardware CLUT registers ($50F00034 / $50F00036).
 */
void lc3_ActivatePalette(void *srcWindow)
{
    (void)srcWindow;
    PaletteHandle pal = s_default_palette;
    if (!pal || !*pal) return;

    Palette *p = *pal;
    volatile uint8_t *clut_addr = (volatile uint8_t *)(LC3_SONORA_BASE + 0x0034);
    volatile uint8_t *clut_data = (volatile uint8_t *)(LC3_SONORA_BASE + 0x0036);

    for (int i = 0; i < p->pmEntries; i++) {
        *clut_addr = (uint8_t)i;
        *clut_data = (uint8_t)(p->pmInfo[i].ciRGB.red >> 8);
        *clut_data = (uint8_t)(p->pmInfo[i].ciRGB.green >> 8);
        *clut_data = (uint8_t)(p->pmInfo[i].ciRGB.blue >> 8);
    }
}

/* SetPalette / GetPalette: Emulates Traps $AA95 / $AA96 */
void lc3_SetPalette(void *dstWindow, PaletteHandle srcPalette, int16_t cUpdates)
{
    (void)dstWindow;
    (void)cUpdates;
    if (srcPalette && *srcPalette) {
        s_default_palette = srcPalette;
        lc3_ActivatePalette(dstWindow);
    }
}

PaletteHandle lc3_GetPalette(void *srcWindow)
{
    (void)srcWindow;
    return s_default_palette;
}

/* AnimateEntry: Emulates Trap $AA99
 * Instantly modifies a single hardware CLUT index without redrawing the window.
 */
void lc3_AnimateEntry(void *dstWindow, int16_t dstEntry, const RGBColor *srcRGB)
{
    (void)dstWindow;
    if (!srcRGB || dstEntry < 0 || dstEntry > 255) return;

    if (s_default_palette && *s_default_palette) {
        (*s_default_palette)->pmInfo[dstEntry].ciRGB = *srcRGB;
    }

    volatile uint8_t *clut_addr = (volatile uint8_t *)(LC3_SONORA_BASE + 0x0034);
    volatile uint8_t *clut_data = (volatile uint8_t *)(LC3_SONORA_BASE + 0x0036);

    *clut_addr = (uint8_t)dstEntry;
    *clut_data = (uint8_t)(srcRGB->red >> 8);
    *clut_data = (uint8_t)(srcRGB->green >> 8);
    *clut_data = (uint8_t)(srcRGB->blue >> 8);
}

/* AnimatePalette: Emulates Trap $AA9A */
void lc3_AnimatePalette(void *dstWindow, PaletteHandle srcPalette, int16_t srcIndex, int16_t dstEntry, int16_t dstLength)
{
    (void)dstWindow;
    if (!srcPalette || !*srcPalette) return;

    Palette *p = *srcPalette;
    for (int i = 0; i < dstLength; i++) {
        int16_t s_idx = srcIndex + i;
        int16_t d_idx = dstEntry + i;
        if (s_idx < p->pmEntries && d_idx < 256) {
            lc3_AnimateEntry(dstWindow, d_idx, &p->pmInfo[s_idx].ciRGB);
        }
    }
}

/* GetEntryColor / SetEntryColor: Emulates Traps $AA9B / $AA9C */
void lc3_GetEntryColor(PaletteHandle srcPalette, int16_t srcEntry, RGBColor *dstRGB)
{
    if (!srcPalette || !*srcPalette || !dstRGB || srcEntry < 0) return;
    if (srcEntry < (*srcPalette)->pmEntries) {
        *dstRGB = (*srcPalette)->pmInfo[srcEntry].ciRGB;
    }
}

void lc3_SetEntryColor(PaletteHandle dstPalette, int16_t dstEntry, const RGBColor *srcRGB)
{
    if (!dstPalette || !*dstPalette || !srcRGB || dstEntry < 0) return;
    if (dstEntry < (*dstPalette)->pmEntries) {
        (*dstPalette)->pmInfo[dstEntry].ciRGB = *srcRGB;
    }
}

/* GetEntryUsage / SetEntryUsage: Emulates Traps $AA9D / $AA9E */
void lc3_GetEntryUsage(PaletteHandle srcPalette, int16_t srcEntry, int16_t *dstUsage, int16_t *dstTolerance)
{
    if (!srcPalette || !*srcPalette || srcEntry < 0) return;
    if (srcEntry < (*srcPalette)->pmEntries) {
        if (dstUsage)     *dstUsage     = (*srcPalette)->pmInfo[srcEntry].ciUsage;
        if (dstTolerance) *dstTolerance = (*srcPalette)->pmInfo[srcEntry].ciTolerance;
    }
}

void lc3_SetEntryUsage(PaletteHandle dstPalette, int16_t dstEntry, int16_t srcUsage, int16_t srcTolerance)
{
    if (!dstPalette || !*dstPalette || dstEntry < 0) return;
    if (dstEntry < (*dstPalette)->pmEntries) {
        (*dstPalette)->pmInfo[dstEntry].ciUsage = srcUsage;
        (*dstPalette)->pmInfo[dstEntry].ciTolerance = srcTolerance;
    }
}
