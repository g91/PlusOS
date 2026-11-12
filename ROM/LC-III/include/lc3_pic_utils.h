/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Picture Utilities Package 15
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PIC_UTILS_H
#define LC3_PIC_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#define PIC_NO_ERR           0
#define PIC_PARAM_ERR      -50

#define RETURN_COLOR_TABLE   (1 << 0)
#define RETURN_PALETTE       (1 << 1)
#define RECORD_COMMENTS      (1 << 2)
#define RECORD_FONTS         (1 << 3)

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} PURect;

typedef struct {
    int16_t version;
    uint32_t uniqueColors;
    void *theColorTable;
    void *thePalette;
    void *theFontList;
    void *theCommentList;
    uint32_t countColors;
    uint32_t countFonts;
    uint32_t countComments;
    int16_t depth;
    PURect sourceRect;
} PictInfo;

/* Picture Utilities API */
int16_t lc3_get_pict_info(void *thePictHandle, PictInfo *thePictInfo, int16_t verb, int16_t colorsRequested, int16_t colorPickMethod, int16_t version);
int16_t lc3_get_pixmap_info(void *thePixMapHandle, PictInfo *thePictInfo, int16_t verb, int16_t colorsRequested, int16_t colorPickMethod, int16_t version);
int16_t lc3_new_pict_info(PictInfo **thePictInfo, int16_t verb, int16_t colorsRequested, int16_t colorPickMethod, int16_t version);
int16_t lc3_record_pict_info(PictInfo *thePictInfo, void *thePictHandle);
int16_t lc3_record_pixmap_info(PictInfo *thePictInfo, void *thePixMapHandle);
int16_t lc3_retrieve_pict_info(PictInfo *thePictInfo, PictInfo *theReturnedInfo, int16_t colorsRequested);
int16_t lc3_dispose_pict_info(PictInfo *thePictInfo);

/* Package 15 Dispatcher ($A832) */
int16_t lc3_pack15_dispatch(int16_t selector, void *params);

#endif /* LC3_PIC_UTILS_H */
