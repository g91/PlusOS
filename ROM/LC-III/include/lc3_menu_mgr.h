/* lc3_menu_mgr.h - Macintosh LC III Toolbox Menu Manager
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x016D20 - 0x01817E
 */
#ifndef LC3_MENU_MGR_H
#define LC3_MENU_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_MENU_ITEMS 32
#define MAX_MENUS      16

/* Menu Item Record */
typedef struct {
    char     itemText[32];      /* Item title */
    uint8_t  itemIcon;          /* Icon number (0 = none) */
    char     itemCmd;           /* Command shortcut key */
    char     itemMark;          /* Checkmark character */
    uint8_t  itemStyle;         /* Bold, Italic, etc. */
    bool     isEnabled;
} LC3MenuItem;

/* Menu Record */
typedef struct LC3MenuRecord {
    int16_t     menuID;         /* Menu resource ID */
    int16_t     menuWidth;      /* Width in pixels */
    int16_t     menuHeight;     /* Height in pixels */
    void      **menuProc;       /* MDEF handle */
    int32_t     enableFlags;    /* Bitmask of enabled items */
    char        menuTitle[32];  /* Pascal title */
    uint16_t    itemCount;
    LC3MenuItem items[MAX_MENU_ITEMS];
} LC3MenuRecord;

typedef LC3MenuRecord **LC3MenuHandle;

/* Menu Manager Low-Memory Globals */
#define LM_MENULIST          (*(volatile uint32_t *)0x0A1C)
#define LM_MENUDISABLE       (*(volatile int32_t  *)0x0B54)
#define LM_THEMENU           (*(volatile int16_t  *)0x0A26)

/* Menu Manager API */
void          lc3_menu_init(void);
LC3MenuHandle lc3_menu_new(int16_t menuID, const char *title);
void          lc3_menu_dispose(LC3MenuHandle menu);
void          lc3_menu_append(LC3MenuHandle menu, const char *data);
void          lc3_menu_insert_item(LC3MenuHandle menu, const char *itemString, int16_t afterItem);
void          lc3_menu_delete_item(LC3MenuHandle menu, int16_t item);
void          lc3_menu_set_item(LC3MenuHandle menu, int16_t item, const char *itemString);
void          lc3_menu_check_item(LC3MenuHandle menu, int16_t item, bool checked);
void          lc3_menu_enable_item(LC3MenuHandle menu, int16_t item);
void          lc3_menu_disable_item(LC3MenuHandle menu, int16_t item);
int16_t       lc3_menu_count_items(LC3MenuHandle menu);
void          lc3_menu_draw_bar(void);
int32_t       lc3_menu_select(int16_t startH, int16_t startV);
void          lc3_menu_hilite(int16_t menuID);

#endif /* LC3_MENU_MGR_H */
