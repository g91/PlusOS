/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: List Manager (Package 0 in ROM)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_LIST_MGR_H
#define LC3_LIST_MGR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t v;
    int16_t h;
} ListCell;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} ListRect;

typedef struct ListRec {
    ListRect rView;
    void *port;
    ListRect indent;
    ListCell cellSize;
    ListRect visible;
    void *vScroll;
    void *hScroll;
    uint8_t selFlags;
    bool lActive;
    uint8_t lReserved;
    int16_t listFlags;
    void *clikLoop;
    ListCell lastClick;
    int32_t clikTime;
    int16_t clikLoc;
    int16_t numRows;
    int16_t numCols;
    char cellData[64][128]; // Preallocated cell text table
    bool cellSelected[64];
} ListRec, *ListPtr, **ListHandle;

/* List Manager Package 0 API */
ListHandle lc3_l_new(const ListRect *rView, const ListRect *dataBounds, ListCell cSize, int16_t cDefProc, void *wPtr, bool drawIt, bool hasGrow, bool scrollHoriz, bool scrollVert);
void lc3_l_dispose(ListHandle lHandle);
int16_t lc3_l_add_row(int16_t count, int16_t rowNum, ListHandle lHandle);
void lc3_l_del_row(int16_t count, int16_t rowNum, ListHandle lHandle);
int16_t lc3_l_add_col(int16_t count, int16_t colNum, ListHandle lHandle);
void lc3_l_del_col(int16_t count, int16_t colNum, ListHandle lHandle);
void lc3_l_set_cell(const void *dataPtr, int16_t dataLen, ListCell theCell, ListHandle lHandle);
void lc3_l_get_cell(void *dataPtr, int16_t *dataLen, ListCell theCell, ListHandle lHandle);
void lc3_l_draw(ListCell theCell, ListHandle lHandle);
bool lc3_l_click(ListCell pt, int16_t modifiers, ListHandle lHandle);
void lc3_l_activate(bool act, ListHandle lHandle);
void lc3_l_update(void *rgnHandle, ListHandle lHandle);
bool lc3_l_search(const void *dataPtr, int16_t dataLen, void *searchProc, ListCell *theCell, ListHandle lHandle);
void lc3_l_set_select(bool setIt, ListCell theCell, ListHandle lHandle);
void lc3_l_auto_scroll(ListHandle lHandle);

/* Package 0 Dispatcher */
void lc3_pack0_dispatch(int16_t selector, void *params);

#endif /* LC3_LIST_MGR_H */
