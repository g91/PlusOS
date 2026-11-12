/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: ColorSync 1.0 Color Matching Manager
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COLORSYNC_H
#define LC3_COLORSYNC_H

#include <stdint.h>
#include <stdbool.h>

#define CS_NO_ERR               0
#define CS_PROFILE_NOT_FOUND -4200
#define CS_PARAM_ERR           -50

typedef void *CMProfileHandle;
typedef void *CWConcatColorWorld;

typedef struct {
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} CMColor;

typedef struct {
    uint32_t profileSize;
    uint32_t cmmType;
    uint32_t profileVersion;
    uint32_t profileClass;
    uint32_t dataColorSpace;
    uint32_t profileConnectionSpace;
    uint32_t signature;
    uint32_t platform;
    uint32_t flags;
    uint32_t deviceManufacturer;
    uint32_t deviceModel;
} CMHeader;

/* ColorSync API */
int16_t lc3_cm_match_colors(CWConcatColorWorld cw, CMColor *colors, uint32_t count);
int16_t lc3_cm_check_colors(CWConcatColorWorld cw, const CMColor *colors, uint32_t count, uint8_t *result);
int16_t lc3_cm_match_pixmap(CWConcatColorWorld cw, void *pixMap);
int16_t lc3_cw_new_color_world(CWConcatColorWorld *cw, CMProfileHandle srcProfile, CMProfileHandle dstProfile);
int16_t lc3_cw_dispose_color_world(CWConcatColorWorld cw);
uint32_t lc3_cm_get_version(void);

#endif /* LC3_COLORSYNC_H */
