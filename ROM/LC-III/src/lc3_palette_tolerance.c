/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Palette Tolerance & Color Arbitration Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_palette_tolerance.h"
#include <string.h>

static bool sClutSlotReserved[256];

uint32_t lc3_pal_calc_color_distance(const RGBColor *c1, const RGBColor *c2)
{
    if (!c1 || !c2) return 0xFFFFFFFF;

    /* Scaled weighted Euclidean distance: 2*dR^2 + 4*dG^2 + dB^2 */
    int32_t dr = (int32_t)(c1->red >> 8) - (int32_t)(c2->red >> 8);
    int32_t dg = (int32_t)(c1->green >> 8) - (int32_t)(c2->green >> 8);
    int32_t db = (int32_t)(c1->blue >> 8) - (int32_t)(c2->blue >> 8);

    return (uint32_t)(2 * dr * dr + 4 * dg * dg + db * db);
}

int16_t lc3_pal_find_tolerant_match(const RGBColor *target, const RGBColor *clutEntries, uint16_t clutCount, int16_t tolerance, int16_t *matchedIndex)
{
    if (!target || !clutEntries || !matchedIndex || clutCount == 0) return PAL_PARAM_ERR;

    uint32_t bestDist = 0xFFFFFFFF;
    int16_t bestIdx = -1;

    for (uint16_t i = 0; i < clutCount; ++i) {
        uint32_t dist = lc3_pal_calc_color_distance(target, &clutEntries[i]);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = (int16_t)i;
        }
    }

    /* Convert 16-bit tolerance parameter to distance threshold */
    uint32_t maxAllowedDist = (uint32_t)((tolerance >> 8) * (tolerance >> 8) * 7);

    if (bestDist <= maxAllowedDist || tolerance == 0x7FFF) {
        *matchedIndex = bestIdx;
        return PAL_NO_ERR;
    }

    *matchedIndex = bestIdx;
    return PAL_NO_MATCH_ERR;
}

int16_t lc3_pal_reserve_entry(int16_t clutIndex)
{
    if (clutIndex < 0 || clutIndex >= 256) return PAL_PARAM_ERR;
    sClutSlotReserved[clutIndex] = true;
    return PAL_NO_ERR;
}

int16_t lc3_pal_release_entry(int16_t clutIndex)
{
    if (clutIndex < 0 || clutIndex >= 256) return PAL_PARAM_ERR;
    sClutSlotReserved[clutIndex] = false;
    return PAL_NO_ERR;
}

int16_t lc3_pal_build_mapping(const PaletteEntry *pal, uint16_t palCount, const RGBColor *clutEntries, uint16_t clutCount, int16_t *outMap)
{
    if (!pal || !clutEntries || !outMap) return PAL_PARAM_ERR;

    for (uint16_t i = 0; i < palCount; ++i) {
        int16_t match = 0;
        lc3_pal_find_tolerant_match(&pal[i].ciRGB, clutEntries, clutCount, pal[i].ciTolerance, &match);
        outMap[i] = match;
    }

    return PAL_NO_ERR;
}

bool lc3_pal_check_inhibit(uint16_t inhibitFlags, const RGBColor *c)
{
    if (!c) return false;

    /* Check for pure black/white inhibition */
    bool isBlack = (c->red == 0 && c->green == 0 && c->blue == 0);
    bool isWhite = (c->red == 0xFFFF && c->green == 0xFFFF && c->blue == 0xFFFF);

    if ((inhibitFlags & 0x01) && isBlack) return true;
    if ((inhibitFlags & 0x02) && isWhite) return true;

    return false;
}
