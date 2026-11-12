/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color Menu Bar & Menu Definition Function (MBDF 0 / MDEF 0 DefProc)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_MDEF_COLOR_H
#define LC3_MDEF_COLOR_H

#include <stdint.h>
#include <stdbool.h>

#define MDEF_NO_ERR         0
#define MDEF_PARAM_ERR    -50

/* MDEF Messages */
#define mDrawMsg            0
#define mChooseMsg          1
#define mSizeMsg            2
#define mPopUpMsg           3

typedef struct MenuItemRecord {
    char itemText[64];
    uint8_t iconNum;
    uint8_t keyboardEquiv;
    uint8_t markChar;
    uint8_t itemModifier;
    bool enabled;
} MenuItemRecord;

typedef struct MenuRecord {
    int16_t menuID;
    int16_t menuWidth;
    int16_t menuHeight;
    char menuTitle[32];
    uint32_t enableFlags;
    uint16_t numItems;
    MenuItemRecord items[32];
} MenuRecord;

/* Color MBDF & MDEF API */
void lc3_mbdf_draw_bar(void *fbBase, int16_t screenWidth, int16_t rowBytes, int16_t depth);
void lc3_mbdf_draw_title(const char *title, int16_t left, int16_t right, bool hilited, void *fbBase, int16_t rowBytes);
void lc3_mdef_draw_item(const MenuItemRecord *item, int16_t top, int16_t left, int16_t width, bool hilited, void *fbBase, int16_t rowBytes);
void lc3_mdef_draw_menu(const MenuRecord *menu, int16_t top, int16_t left, void *fbBase, int16_t rowBytes);
int16_t lc3_mdef_choose(const MenuRecord *menu, int16_t menuTop, int16_t menuLeft, int16_t mouseX, int16_t mouseY);
int32_t lc3_mdef_entry(int16_t message, MenuRecord *menu, void *menuRect, int32_t hitPt, int16_t *whichItem);

#endif /* LC3_MDEF_COLOR_H */
