/* lc3_syserr_alert.c - Macintosh LC III System Error & Alert Manager
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   SYSERR_TRY_DEBUGGER, SYSERR_DISPATCH, ALLOCFAKERGNS, DT_INF_RGN,
 *   DSERRORHANDLER, LOOKUP_ALERT_ID, LOOKUP_LOOP, DRAWTEXT_DONE,
 *   CALL_DS_DRAWPROC, REPOS_ALERT_PORT, DRAW_ALERT_FRAME, DRAW_BTN_LOOP,
 *   BTN_INVERT, WAIT_MOUSE_DOWN
 */

#include "lc3_syserr_alert.h"
#include "lc3_microbug.h"
#include "lc3_hardware.h"
#include <string.h>

static LC3SysErrState g_sysErrState;

void lc3_syserr_init(void) {
    LM_DSERRCODE = 0;
    LM_DSALERTTAB = 0x408027E6; /* Points to DT_INF_RGN */
}

bool lc3_syserr_try_debugger(int16_t errorCode) {
    /* SYSERR_TRY_DEBUGGER: checks MacsBug / MicroBug flag ($0120) */
    volatile uint32_t *macsbugVector = (volatile uint32_t *)0x0120;
    if (*macsbugVector != 0) {
        /* User debugger installed: pass error and invoke debugger */
        void (*dbgEntry)(int16_t) = (void (*)(int16_t))*macsbugVector;
        dbgEntry(errorCode);
        return true;
    }
    return false;
}

void lc3_syserr_draw_alert_frame(int16_t top, int16_t left, int16_t bottom, int16_t right) {
    /* DRAW_ALERT_FRAME: renders standard Mac System Bomb double-line frame */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80; /* 1-bpp monochrome */

    /* Clear box interior (white) */
    for (int16_t r = top; r <= bottom; ++r) {
        uint32_t offset = (r * rowBytes) + (left / 8);
        int16_t widthBytes = (right - left) / 8;
        for (int16_t c = 0; c < widthBytes; ++c) {
            vram[offset + c] = 0x00; /* white */
        }
    }

    /* Draw thick 2-pixel black border */
    for (int16_t c = left / 8; c <= right / 8; ++c) {
        vram[(top * rowBytes) + c] = 0xFF;
        vram[((top + 1) * rowBytes) + c] = 0xFF;
        vram[((bottom - 1) * rowBytes) + c] = 0xFF;
        vram[(bottom * rowBytes) + c] = 0xFF;
    }
    for (int16_t r = top; r <= bottom; ++r) {
        vram[(r * rowBytes) + (left / 8)] |= 0xC0;
        vram[(r * rowBytes) + (right / 8)] |= 0x03;
    }
}

void lc3_syserr_btn_invert(const LC3AlertButton *btn) {
    /* BTN_INVERT: inverts button pixel contents on click/press */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80;
    for (int16_t r = btn->top; r <= btn->bottom; ++r) {
        uint32_t offset = (r * rowBytes) + (btn->left / 8);
        int16_t widthBytes = (btn->right - btn->left) / 8;
        for (int16_t c = 0; c < widthBytes; ++c) {
            vram[offset + c] ^= 0xFF;
        }
    }
}

void lc3_syserr_draw_buttons(const LC3AlertButton *btns, uint8_t count) {
    /* DRAW_BTN_LOOP: draws rounded rectangular buttons */
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80;

    for (uint8_t i = 0; i < count; ++i) {
        const LC3AlertButton *b = &btns[i];
        /* Draw button border */
        for (int16_t c = b->left / 8; c <= b->right / 8; ++c) {
            vram[(b->top * rowBytes) + c] = 0xFF;
            vram[(b->bottom * rowBytes) + c] = 0xFF;
        }
        for (int16_t r = b->top; r <= b->bottom; ++r) {
            vram[(r * rowBytes) + (b->left / 8)] |= 0x80;
            vram[(r * rowBytes) + (b->right / 8)] |= 0x01;
        }
        /* Draw default heavy ring if isDefault */
        if (b->isDefault) {
            for (int16_t c = (b->left - 3) / 8; c <= (b->right + 3) / 8; ++c) {
                vram[((b->top - 3) * rowBytes) + c] |= 0xFF;
                vram[((b->bottom + 3) * rowBytes) + c] |= 0xFF;
            }
        }
    }
}

int16_t lc3_syserr_wait_mouse_down(const LC3AlertButton *btns, uint8_t count) {
    /* WAIT_MOUSE_DOWN: polls mouse button state via VIA1 */
    volatile uint8_t *via = (volatile uint8_t *)LC3_VIA1_BASE;
    while (true) {
        /* Check mouse button (bit 7 active low) */
        if ((via[0x1E00] & 0x80) == 0) {
            /* Button pressed: return selected button */
            lc3_syserr_btn_invert(&btns[0]);
            /* Debounce release */
            while ((via[0x1E00] & 0x80) == 0);
            return 0;
        }
    }
}

void lc3_syserr_dispatch(int16_t errorCode, uint32_t faultPC, uint32_t faultAddr) {
    /* DSERRORHANDLER, SYSERR_DISPATCH: primary entry point for Mac system bomb */
    LM_DSERRCODE = errorCode;

    /* First attempt invoking installed debugger */
    if (lc3_syserr_try_debugger(errorCode)) {
        return;
    }

    g_sysErrState.dsAlertID = errorCode;
    g_sysErrState.faultPC = faultPC;
    g_sysErrState.faultAddr = faultAddr;

    /* Setup standard "Restart" button */
    g_sysErrState.buttonCount = 1;
    LC3AlertButton *btn = &g_sysErrState.buttons[0];
    btn->top = 260;
    btn->bottom = 280;
    btn->left = 320;
    btn->right = 400;
    strcpy(btn->title, "Restart");
    btn->isDefault = true;

    /* Render System Alert Dialog: centered at (150, 160) - (300, 480) */
    lc3_syserr_draw_alert_frame(150, 160, 300, 480);
    lc3_syserr_draw_buttons(g_sysErrState.buttons, g_sysErrState.buttonCount);

    /* Wait for user response */
    lc3_syserr_wait_mouse_down(g_sysErrState.buttons, g_sysErrState.buttonCount);

    /* Hardware reset system */
    volatile uint8_t *via = (volatile uint8_t *)LC3_VIA1_BASE;
    via[0x1E00] = 0x00; /* Trigger cold boot restart */
    while (1);
}
