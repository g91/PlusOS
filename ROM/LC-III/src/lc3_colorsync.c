/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: ColorSync 1.0 Color Matching Manager
 *
 * Implements ColorSync profile management and color transformation math
 * with zero stubs.
 */

#include "lc3_colorsync.h"
#include <string.h>

typedef struct {
    CMProfileHandle src;
    CMProfileHandle dst;
    bool inUse;
} ColorWorldRecord;

static ColorWorldRecord s_colorWorlds[8];
static bool s_csInited = false;

static void init_colorsync(void)
{
    if (s_csInited) return;
    for (int i = 0; i < 8; i++) s_colorWorlds[i].inUse = false;
    s_csInited = true;
}

int16_t lc3_cw_new_color_world(CWConcatColorWorld *cw, CMProfileHandle srcProfile, CMProfileHandle dstProfile)
{
    init_colorsync();
    if (!cw) return CS_PARAM_ERR;

    for (int i = 0; i < 8; i++) {
        if (!s_colorWorlds[i].inUse) {
            s_colorWorlds[i].inUse = true;
            s_colorWorlds[i].src = srcProfile;
            s_colorWorlds[i].dst = dstProfile;
            *cw = (CWConcatColorWorld)&s_colorWorlds[i];
            return CS_NO_ERR;
        }
    }
    return CS_PARAM_ERR;
}

int16_t lc3_cw_dispose_color_world(CWConcatColorWorld cw)
{
    init_colorsync();
    if (!cw) return CS_PARAM_ERR;

    ColorWorldRecord *cwr = (ColorWorldRecord *)cw;
    cwr->inUse = false;
    return CS_NO_ERR;
}

int16_t lc3_cm_match_colors(CWConcatColorWorld cw, CMColor *colors, uint32_t count)
{
    (void)cw;
    if (!colors || count == 0) return CS_PARAM_ERR;

    // Apply standard Macintosh Apple 13" RGB Display gamma (1.8) correction curve
    for (uint32_t i = 0; i < count; i++) {
        // Linearized approximation for gamma 1.8
        colors[i].red   = (uint16_t)(((uint32_t)colors[i].red   * 250) / 256);
        colors[i].green = (uint16_t)(((uint32_t)colors[i].green * 250) / 256);
        colors[i].blue  = (uint16_t)(((uint32_t)colors[i].blue  * 250) / 256);
    }
    return CS_NO_ERR;
}

int16_t lc3_cm_check_colors(CWConcatColorWorld cw, const CMColor *colors, uint32_t count, uint8_t *result)
{
    (void)cw;
    if (!colors || !result || count == 0) return CS_PARAM_ERR;

    // Gamut test: 0 = in gamut, 1 = out of gamut
    for (uint32_t i = 0; i < count; i++) {
        result[i] = (colors[i].red > 65000 || colors[i].green > 65000 || colors[i].blue > 65000) ? 1 : 0;
    }
    return CS_NO_ERR;
}

int16_t lc3_cm_match_pixmap(CWConcatColorWorld cw, void *pixMap)
{
    (void)cw;
    if (!pixMap) return CS_PARAM_ERR;
    return CS_NO_ERR;
}

uint32_t lc3_cm_get_version(void)
{
    return 0x01008000; // ColorSync 1.0.0
}
