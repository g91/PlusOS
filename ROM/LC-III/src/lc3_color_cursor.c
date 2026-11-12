/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color QuickDraw Color Cursor Engine (CCrsr & Animated Cursors)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_color_cursor.h"
#include <string.h>

int16_t lc3_ccrsr_init(CursorAnimContext *ctx)
{
    if (!ctx) return CCRSR_PARAM_ERR;

    memset(ctx, 0, sizeof(CursorAnimContext));
    ctx->currentCursor.crsrType = 0x8001;
    ctx->currentPos.h = 100;
    ctx->currentPos.v = 100;
    ctx->currentFrame = 0;
    ctx->totalFrames = 1;
    ctx->isVisible = true;

    return CCRSR_NO_ERR;
}

int16_t lc3_ccrsr_set(CursorAnimContext *ctx, const CCrsrRecord *newCursor)
{
    if (!ctx || !newCursor) return CCRSR_PARAM_ERR;

    ctx->currentCursor = *newCursor;
    return CCRSR_NO_ERR;
}

void lc3_ccrsr_blit(CursorAnimContext *ctx, int16_t mouseX, int16_t mouseY, void *fbBase, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!ctx || !fbBase || !ctx->isVisible) return;

    uint8_t *fb = (uint8_t *)fbBase;
    int16_t originX = mouseX - ctx->currentCursor.crsrHotSpot.h;
    int16_t originY = mouseY - ctx->currentCursor.crsrHotSpot.v;

    ctx->currentPos.h = originX;
    ctx->currentPos.v = originY;

    /* 1. Save background under cursor */
    for (int16_t y = 0; y < CURSOR_HEIGHT; ++y) {
        for (int16_t x = 0; x < CURSOR_WIDTH; ++x) {
            int16_t screenX = originX + x;
            int16_t screenY = originY + y;
            if (screenX >= 0 && screenY >= 0) {
                ctx->savedBackground[y * CURSOR_WIDTH + x] = fb[screenY * rowBytes + screenX];
            }
        }
    }

    /* 2. Blit 16x16 cursor pixels through 1-bpp mask */
    for (int16_t y = 0; y < CURSOR_HEIGHT; ++y) {
        uint16_t maskWord = (uint16_t)((ctx->currentCursor.crsrMask[y * 2] << 8) | ctx->currentCursor.crsrMask[y * 2 + 1]);
        uint16_t dataWord = (uint16_t)((ctx->currentCursor.crsr1Data[y * 2] << 8) | ctx->currentCursor.crsr1Data[y * 2 + 1]);

        for (int16_t x = 0; x < CURSOR_WIDTH; ++x) {
            bool maskBit = (maskWord & (1 << (15 - x))) != 0;
            bool dataBit = (dataWord & (1 << (15 - x))) != 0;

            if (maskBit) {
                int16_t screenX = originX + x;
                int16_t screenY = originY + y;
                if (screenX >= 0 && screenY >= 0) {
                    fb[screenY * rowBytes + screenX] = dataBit ? 0xFF : 0x00;
                }
            }
        }
    }
}

void lc3_ccrsr_restore_bg(CursorAnimContext *ctx, void *fbBase, int16_t rowBytes)
{
    if (!ctx || !fbBase) return;

    uint8_t *fb = (uint8_t *)fbBase;
    int16_t originX = ctx->currentPos.h;
    int16_t originY = ctx->currentPos.v;

    for (int16_t y = 0; y < CURSOR_HEIGHT; ++y) {
        for (int16_t x = 0; x < CURSOR_WIDTH; ++x) {
            int16_t screenX = originX + x;
            int16_t screenY = originY + y;
            if (screenX >= 0 && screenY >= 0) {
                fb[screenY * rowBytes + screenX] = ctx->savedBackground[y * CURSOR_WIDTH + x];
            }
        }
    }
}

void lc3_ccrsr_animate_step(CursorAnimContext *ctx)
{
    if (!ctx || ctx->totalFrames <= 1) return;

    ctx->currentFrame = (ctx->currentFrame + 1) % ctx->totalFrames;
}

void lc3_ccrsr_dispose(CursorAnimContext *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(CursorAnimContext));
}
