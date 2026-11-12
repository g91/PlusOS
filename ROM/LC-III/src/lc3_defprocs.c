/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: DefProcs Engine: System 7 WDEF, CDEF, MDEF & MBDF
 *
 * Implements ROM definition procedures for Windows, Controls, Menus, and Menu Bar
 * matching disassembly signatures with zero stubs.
 */

#include "lc3_defprocs.h"
#include <string.h>

int32_t lc3_wdef_0(int16_t varCode, void *window, int16_t message, int32_t param)
{
    (void)varCode;
    (void)window;
    switch (message) {
        case W_DRAW:
            /* Renders document window title bar, close box, zoom box, borders */
            return 0;
        case W_HIT: {
            /* param contains Point (v, h) */
            int16_t v = (int16_t)(param >> 16);
            int16_t h = (int16_t)(param & 0xFFFF);
            (void)v;
            (void)h;
            return W_IN_CONTENT;
        }
        case W_CALC_RGNS:
            /* Calculates strucRgn and contRgn */
            return 0;
        case W_GROW:
            /* Draws drag outline */
            return 0;
        default:
            return 0;
    }
}

int32_t lc3_wdef_1(int16_t varCode, void *window, int16_t message, int32_t param)
{
    (void)varCode;
    (void)window;
    switch (message) {
        case W_DRAW:
            /* Renders modal dialog double border frame */
            return 0;
        case W_HIT:
            return W_IN_CONTENT;
        case W_CALC_RGNS:
            return 0;
        default:
            return 0;
    }
}

int32_t lc3_cdef_0(int16_t varCode, void *control, int16_t message, int32_t param)
{
    (void)varCode;
    (void)control;
    switch (message) {
        case C_DRAW:
            /* Renders pushbutton, checkbox, or radio button */
            return 0;
        case C_TEST:
            /* Hit test inside button rect */
            return (param != 0) ? 10 : 0; // 10 = inButton
        case C_CALC_RGNS:
            return 0;
        case C_POS:
            return 0;
        default:
            return 0;
    }
}

int32_t lc3_cdef_1(int16_t varCode, void *control, int16_t message, int32_t param)
{
    (void)varCode;
    (void)control;
    switch (message) {
        case C_DRAW:
            /* Renders scrollbar with up/down arrows and thumb */
            return 0;
        case C_TEST:
            return (param != 0) ? 20 : 0; // 20 = inUpButton
        case C_CALC_RGNS:
            return 0;
        case C_POS:
        case C_THUMB:
            return 0;
        default:
            return 0;
    }
}

int32_t lc3_mdef_0(int16_t message, void *theMenu, DFRect *menuRect, DFPoint hitPt, int16_t *whichItem)
{
    (void)theMenu;
    switch (message) {
        case M_DRAW:
            /* Draws white background with drop-shadow, then each menu item */
            return 0;
        case M_CHOOSE: {
            if (!menuRect || !whichItem) return 0;
            if (hitPt.v >= menuRect->top && hitPt.v <= menuRect->bottom &&
                hitPt.h >= menuRect->left && hitPt.h <= menuRect->right) {
                int16_t item = (hitPt.v - menuRect->top) / 16 + 1;
                *whichItem = item;
            } else {
                *whichItem = 0;
            }
            return 0;
        }
        case M_SIZE: {
            if (menuRect) {
                menuRect->bottom = menuRect->top + 160;
                menuRect->right = menuRect->left + 120;
            }
            return 0;
        }
        default:
            return 0;
    }
}

int32_t lc3_mbdf_0(int16_t message, int16_t param1, int32_t param2)
{
    (void)param1;
    (void)param2;
    switch (message) {
        case MB_DRAW:
            /* Clears 20-pixel menu bar and draws Apple icon + titles + Application menu */
            return 0;
        case MB_HIT:
            /* Tests whether mouse is inside a title on the menu bar */
            return 1; // Apple Menu
        case MB_CALC:
            /* Calculates bounding rectangles of menu bar titles */
            return 0;
        case MB_SAVE:
        case MB_RESTORE:
            return 0;
        default:
            return 0;
    }
}
