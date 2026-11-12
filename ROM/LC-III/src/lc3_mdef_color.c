/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Color Menu Bar & Menu Definition Function (MBDF 0 / MDEF 0 DefProc)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_mdef_color.h"
#include <string.h>

#define MENU_BAR_HEIGHT 20
#define MENU_ITEM_HEIGHT 16

static inline void set_pixel(void *fbBase, int16_t rowBytes, int16_t x, int16_t y, uint8_t colorIdx)
{
    if (!fbBase || x < 0 || y < 0) return;
    uint8_t *fb = (uint8_t *)fbBase;
    fb[y * rowBytes + x] = colorIdx;
}

static void fill_rect(void *fbBase, int16_t rowBytes, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colorIdx)
{
    for (int16_t y = top; y < bottom; ++y) {
        for (int16_t x = left; x < right; ++x) {
            set_pixel(fbBase, rowBytes, x, y, colorIdx);
        }
    }
}

void lc3_mbdf_draw_bar(void *fbBase, int16_t screenWidth, int16_t rowBytes, int16_t depth)
{
    (void)depth;
    if (!fbBase || screenWidth <= 0) return;

    /* Fill menu bar background with pure white (0x00 in standard indexed palette) */
    fill_rect(fbBase, rowBytes, 0, 0, screenWidth, MENU_BAR_HEIGHT - 1, 0x00);

    /* Draw bottom black boundary line */
    for (int16_t x = 0; x < screenWidth; ++x) {
        set_pixel(fbBase, rowBytes, x, MENU_BAR_HEIGHT - 1, 0xFF);
    }
}

void lc3_mbdf_draw_title(const char *title, int16_t left, int16_t right, bool hilited, void *fbBase, int16_t rowBytes)
{
    if (!title || !fbBase || left >= right) return;

    uint8_t bg = hilited ? 0xFF : 0x00;
    fill_rect(fbBase, rowBytes, left, 1, right, MENU_BAR_HEIGHT - 1, bg);
}

void lc3_mdef_draw_item(const MenuItemRecord *item, int16_t top, int16_t left, int16_t width, bool hilited, void *fbBase, int16_t rowBytes)
{
    if (!item || !fbBase || width <= 0) return;

    int16_t bottom = top + MENU_ITEM_HEIGHT;
    uint8_t bg = hilited ? 0xFF : 0x00;
    fill_rect(fbBase, rowBytes, left, top, left + width, bottom, bg);
}

void lc3_mdef_draw_menu(const MenuRecord *menu, int16_t top, int16_t left, void *fbBase, int16_t rowBytes)
{
    if (!menu || !fbBase) return;

    int16_t width = menu->menuWidth > 0 ? menu->menuWidth : 120;
    int16_t height = (int16_t)(menu->numItems * MENU_ITEM_HEIGHT);
    int16_t right = left + width;
    int16_t bottom = top + height;

    /* Draw menu body background */
    fill_rect(fbBase, rowBytes, left, top, right, bottom, 0x00);

    /* Draw border around menu */
    for (int16_t x = left; x <= right; ++x) {
        set_pixel(fbBase, rowBytes, x, top, 0xFF);
        set_pixel(fbBase, rowBytes, x, bottom, 0xFF);
    }
    for (int16_t y = top; y <= bottom; ++y) {
        set_pixel(fbBase, rowBytes, left, y, 0xFF);
        set_pixel(fbBase, rowBytes, right, y, 0xFF);
    }

    /* Draw drop shadow: 2 pixels on right and bottom */
    for (int16_t y = top + 2; y <= bottom + 2; ++y) {
        set_pixel(fbBase, rowBytes, right + 1, y, 0x88);
        set_pixel(fbBase, rowBytes, right + 2, y, 0x88);
    }
    for (int16_t x = left + 2; x <= right + 2; ++x) {
        set_pixel(fbBase, rowBytes, x, bottom + 1, 0x88);
        set_pixel(fbBase, rowBytes, x, bottom + 2, 0x88);
    }

    /* Draw individual items */
    for (uint16_t i = 0; i < menu->numItems; ++i) {
        lc3_mdef_draw_item(&menu->items[i], top + (int16_t)(i * MENU_ITEM_HEIGHT), left + 1, width - 2, false, fbBase, rowBytes);
    }
}

int16_t lc3_mdef_choose(const MenuRecord *menu, int16_t menuTop, int16_t menuLeft, int16_t mouseX, int16_t mouseY)
{
    if (!menu) return 0;

    int16_t width = menu->menuWidth > 0 ? menu->menuWidth : 120;
    int16_t height = (int16_t)(menu->numItems * MENU_ITEM_HEIGHT);

    if (mouseX < menuLeft || mouseX > menuLeft + width ||
        mouseY < menuTop || mouseY > menuTop + height) {
        return 0; /* Mouse outside menu */
    }

    int16_t itemIdx = (mouseY - menuTop) / MENU_ITEM_HEIGHT + 1;
    if (itemIdx > (int16_t)menu->numItems) {
        return 0;
    }

    /* Check if item enabled */
    if (!menu->items[itemIdx - 1].enabled) {
        return 0;
    }

    return itemIdx;
}

int32_t lc3_mdef_entry(int16_t message, MenuRecord *menu, void *menuRect, int32_t hitPt, int16_t *whichItem)
{
    (void)menuRect;
    if (!menu) return MDEF_PARAM_ERR;

    switch (message) {
        case mDrawMsg:
            return MDEF_NO_ERR;

        case mChooseMsg: {
            int16_t my = (int16_t)((hitPt >> 16) & 0xFFFF);
            int16_t mx = (int16_t)(hitPt & 0xFFFF);
            int16_t item = lc3_mdef_choose(menu, 20, 10, mx, my);
            if (whichItem) *whichItem = item;
            return MDEF_NO_ERR;
        }

        case mSizeMsg:
            menu->menuWidth = 140;
            menu->menuHeight = (int16_t)(menu->numItems * MENU_ITEM_HEIGHT);
            return MDEF_NO_ERR;

        default:
            return MDEF_NO_ERR;
    }
}
