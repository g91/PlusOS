/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Color Cursor Engine (CCrsr & Animated Cursors)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COLOR_CURSOR_H
#define LC3_COLOR_CURSOR_H

#include <stdint.h>
#include <stdbool.h>

#define CCRSR_NO_ERR         0
#define CCRSR_PARAM_ERR    -50

#define CURSOR_WIDTH        16
#define CURSOR_HEIGHT       16

typedef struct MacPoint {
    int16_t v;
    int16_t h;
} MacPoint;

typedef struct CCrsrRecord {
    uint16_t crsrType;        /* 0x8001 for color cursor */
    void *crsrMap;            /* PixMap handle */
    void *crsrData;           /* Color pixel data handle */
    void *crsrXData;          /* Expanded color data */
    int16_t crsrXValid;
    int32_t crsrXHandle;
    uint8_t crsr1Data[32];    /* 16x16 1-bpp monochrome data */
    uint8_t crsrMask[32];     /* 16x16 mask */
    MacPoint crsrHotSpot;
    int32_t crsrXTable;
    int32_t crsrID;
} CCrsrRecord, *CCrsrPtr, **CCrsrHandle;

typedef struct CursorAnimContext {
    CCrsrRecord currentCursor;
    uint8_t savedBackground[16 * 16];
    MacPoint currentPos;
    int16_t currentFrame;
    int16_t totalFrames;
    bool isVisible;
} CursorAnimContext;

/* Color Cursor API */
int16_t lc3_ccrsr_init(CursorAnimContext *ctx);
int16_t lc3_ccrsr_set(CursorAnimContext *ctx, const CCrsrRecord *newCursor);
void lc3_ccrsr_blit(CursorAnimContext *ctx, int16_t mouseX, int16_t mouseY, void *fbBase, int16_t rowBytes, int16_t depth);
void lc3_ccrsr_restore_bg(CursorAnimContext *ctx, void *fbBase, int16_t rowBytes);
void lc3_ccrsr_animate_step(CursorAnimContext *ctx);
void lc3_ccrsr_dispose(CursorAnimContext *ctx);

#endif /* LC3_COLOR_CURSOR_H */
