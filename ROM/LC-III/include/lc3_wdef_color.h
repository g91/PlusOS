/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color Window Definition Function (WDEF 128 DefProc)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_WDEF_COLOR_H
#define LC3_WDEF_COLOR_H

#include <stdint.h>
#include <stdbool.h>

#define WDEF_NO_ERR         0
#define WDEF_PARAM_ERR    -50

/* WDEF Message Selectors */
#define wDraw               0
#define wHit                1
#define wCalcRgns           2
#define wNew                3
#define wDispose            4
#define wGrow               5
#define wDrawGIcon          6

/* Hit Code Results */
#define wNoHit              0
#define wInContent          1
#define wInDrag             2
#define wInGrow             3
#define wInGoAway           4
#define wInZoomIn           5
#define wInZoomOut          6

typedef struct MacPoint {
    int16_t v;
    int16_t h;
} MacPoint;

typedef struct MacRect {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} MacRect;

typedef struct ColorWindowRecord {
    MacRect portRect;
    MacRect strucRect;
    MacRect contRect;
    char title[64];
    bool visible;
    bool hilited;
    bool hasCloseBox;
    bool hasZoomBox;
    bool hasGrowBox;
} ColorWindowRecord;

/* Color WDEF API */
void lc3_wdef_draw_frame(const ColorWindowRecord *win, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_wdef_draw_close_box(const ColorWindowRecord *win, bool pressed, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_wdef_draw_zoom_box(const ColorWindowRecord *win, bool pressed, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_wdef_draw_grow_box(const ColorWindowRecord *win, void *fbBase, int16_t rowBytes, int16_t depth);
int16_t lc3_wdef_hit_test(const ColorWindowRecord *win, MacPoint pt);
int32_t lc3_wdef_entry(int16_t varCode, ColorWindowRecord *win, int16_t message, int32_t param);

#endif /* LC3_WDEF_COLOR_H */
