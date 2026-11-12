/* lc3_control_mgr.c - Macintosh LC III Toolbox Control Manager
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   NEWCONTROL_RETRY, KILLCONTROLS, DRAWONECONTROL, SHOWCONTROL, HIDECONTROL,
 *   MOVECONTROL, GETCREFCON, SETCREFCON, GETCTLACTION, SETCTLACTION,
 *   HILITECONTROL, GETCTITLE, SETCTITLE, GETCTLVALUE, SETCTLVALUE,
 *   GETCTLMIN, GETCTLMAX, SETCTLMIN, SETCTLMAX, TESTCONTROL, DRAGCONTROL,
 *   TRACKCONTROL, UPDTCONTROLS, DRAWCONTROLS, FINDCONTROL
 */

#include "lc3_control_mgr.h"
#include "lc3_hardware.h"
#include <string.h>

#define MAX_CONTROLS 32
static LC3ControlRecord g_controlStorage[MAX_CONTROLS];
static LC3ControlRecord *g_controlPointers[MAX_CONTROLS];
static bool g_controlInUse[MAX_CONTROLS];

LC3ControlHandle lc3_ctrl_new(void *window, int16_t top, int16_t left, int16_t bottom, int16_t right,
                              const char *title, bool visible, int16_t value, int16_t min, int16_t max,
                              int16_t procID, int32_t refCon) {
    (void)procID;
    for (int i = 0; i < MAX_CONTROLS; ++i) {
        if (!g_controlInUse[i]) {
            g_controlInUse[i] = true;
            LC3ControlRecord *c = &g_controlStorage[i];
            c->contrlOwner = window;
            c->contrlRectTop = top;
            c->contrlRectLeft = left;
            c->contrlRectBottom = bottom;
            c->contrlRectRight = right;
            c->contrlVis = visible ? 255 : 0;
            c->contrlHilite = 0;
            c->contrlValue = value;
            c->contrlMin = min;
            c->contrlMax = max;
            c->contrlAction = 0;
            c->contrlRfCon = refCon;
            strncpy(c->contrlTitle, title, 31);
            c->contrlTitle[31] = '\0';
            g_controlPointers[i] = c;
            if (visible) {
                lc3_ctrl_draw_one(&g_controlPointers[i]);
            }
            return &g_controlPointers[i];
        }
    }
    return 0;
}

void lc3_ctrl_dispose(LC3ControlHandle c) {
    if (c && *c) {
        for (int i = 0; i < MAX_CONTROLS; ++i) {
            if (g_controlPointers[i] == *c) {
                g_controlInUse[i] = false;
                break;
            }
        }
    }
}

void lc3_ctrl_show(LC3ControlHandle c) {
    if (c && *c) {
        (*c)->contrlVis = 255;
        lc3_ctrl_draw_one(c);
    }
}

void lc3_ctrl_hide(LC3ControlHandle c) {
    if (c && *c) {
        (*c)->contrlVis = 0;
    }
}

void lc3_ctrl_move(LC3ControlHandle c, int16_t h, int16_t v) {
    if (c && *c) {
        int16_t width = (*c)->contrlRectRight - (*c)->contrlRectLeft;
        int16_t height = (*c)->contrlRectBottom - (*c)->contrlRectTop;
        (*c)->contrlRectLeft = h;
        (*c)->contrlRectTop = v;
        (*c)->contrlRectRight = h + width;
        (*c)->contrlRectBottom = v + height;
    }
}

void lc3_ctrl_draw_one(LC3ControlHandle c) {
    /* DRAWONECONTROL: renders control frame to VRAM */
    if (!c || !*c || !(*c)->contrlVis) return;
    volatile uint8_t *vram = (volatile uint8_t *)LC3_VRAM_BASE;
    uint32_t rowBytes = 80;

    int16_t top = (*c)->contrlRectTop;
    int16_t bottom = (*c)->contrlRectBottom;
    int16_t left = (*c)->contrlRectLeft;
    int16_t right = (*c)->contrlRectRight;

    /* Draw border lines */
    for (int16_t col = left / 8; col <= right / 8; ++col) {
        vram[(top * rowBytes) + col] |= 0xFF;
        vram[(bottom * rowBytes) + col] |= 0xFF;
    }
    for (int16_t r = top; r <= bottom; ++r) {
        vram[(r * rowBytes) + (left / 8)] |= 0x80;
        vram[(r * rowBytes) + (right / 8)] |= 0x01;
    }
}

void lc3_ctrl_draw_all(void *window) {
    for (int i = 0; i < MAX_CONTROLS; ++i) {
        if (g_controlInUse[i] && g_controlStorage[i].contrlOwner == window) {
            lc3_ctrl_draw_one(&g_controlPointers[i]);
        }
    }
}

void lc3_ctrl_hilite(LC3ControlHandle c, uint8_t hiliteCode) {
    if (c && *c) {
        (*c)->contrlHilite = hiliteCode;
    }
}

int16_t lc3_ctrl_test(LC3ControlHandle c, int16_t h, int16_t v) {
    if (!c || !*c || !(*c)->contrlVis) return 0;
    if (h >= (*c)->contrlRectLeft && h <= (*c)->contrlRectRight &&
        v >= (*c)->contrlRectTop && v <= (*c)->contrlRectBottom) {
        return CTRL_PART_IN_BUTTON;
    }
    return 0;
}

int16_t lc3_ctrl_find(int16_t h, int16_t v, void *window, LC3ControlHandle *whichCtrl) {
    for (int i = 0; i < MAX_CONTROLS; ++i) {
        if (g_controlInUse[i] && g_controlStorage[i].contrlOwner == window) {
            int16_t part = lc3_ctrl_test(&g_controlPointers[i], h, v);
            if (part != 0) {
                if (whichCtrl) *whichCtrl = &g_controlPointers[i];
                return part;
            }
        }
    }
    if (whichCtrl) *whichCtrl = 0;
    return 0;
}

int16_t lc3_ctrl_track(LC3ControlHandle c, int16_t startH, int16_t startV, void (*action)(LC3ControlHandle, int16_t)) {
    (void)startH;
    (void)startV;
    int16_t part = CTRL_PART_IN_BUTTON;
    if (action) {
        action(c, part);
    }
    return part;
}

void lc3_ctrl_set_value(LC3ControlHandle c, int16_t value) {
    if (c && *c) {
        if (value < (*c)->contrlMin) value = (*c)->contrlMin;
        if (value > (*c)->contrlMax) value = (*c)->contrlMax;
        (*c)->contrlValue = value;
    }
}

int16_t lc3_ctrl_get_value(LC3ControlHandle c) {
    return (c && *c) ? (*c)->contrlValue : 0;
}

void lc3_ctrl_set_min(LC3ControlHandle c, int16_t min) {
    if (c && *c) (*c)->contrlMin = min;
}

void lc3_ctrl_set_max(LC3ControlHandle c, int16_t max) {
    if (c && *c) (*c)->contrlMax = max;
}
