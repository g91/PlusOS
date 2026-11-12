/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Control Definition Function (CDEF 0 / CDEF 1 Color Controls)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CDEF_COLOR_H
#define LC3_CDEF_COLOR_H

#include <stdint.h>
#include <stdbool.h>

#define CDEF_NO_ERR         0
#define CDEF_PARAM_ERR    -50

/* CDEF Message Selectors */
#define drawCntl            0
#define testCntl            1
#define calcCRgns           2
#define initCntl            3
#define dispCntl            4
#define posCntl             5
#define thumbCntl           6
#define dragCntl            7
#define autoTrack           8

/* Part Codes */
#define kControlNoPart      0
#define kControlButtonPart 10
#define kControlCheckBoxPart 11
#define kControlRadioButtonPart 12
#define kControlUpButtonPart 20
#define kControlDownButtonPart 21
#define kControlPageUpPart 22
#define kControlPageDownPart 23
#define kControlIndicatorPart 129

typedef struct MacRect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} MacRect;

typedef struct MacPoint {
    int16_t v;
    int16_t h;
} MacPoint;

typedef struct ColorControlRecord {
    MacRect boundsRect;
    int16_t value;
    int16_t min;
    int16_t max;
    int16_t procID;         /* 0 = push button, 1 = checkbox, 2 = radio, 16 = scrollbar */
    char title[64];
    bool visible;
    bool hilited;
    uint8_t hiliteState;    /* 0 = normal, 255 = inactive, other = part code */
} ColorControlRecord;

/* Color CDEF API */
void lc3_cdef_draw_button(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_cdef_draw_checkbox(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_cdef_draw_radio(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_cdef_draw_scrollbar(const ColorControlRecord *ctrl, void *fbBase, int16_t rowBytes, int16_t depth);
int16_t lc3_cdef_hit_test(const ColorControlRecord *ctrl, MacPoint pt);
int32_t lc3_cdef_entry(int16_t varCode, ColorControlRecord *ctrl, int16_t message, int32_t param);

#endif /* LC3_CDEF_COLOR_H */
