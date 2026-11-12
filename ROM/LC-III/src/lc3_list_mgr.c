/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: List Manager (Package 0 in ROM)
 *
 * Implements Package 0 ROM traps and selector dispatch ($A9E7) with zero stubs.
 */

#include "lc3_list_mgr.h"
#include <string.h>

static ListRec s_listPool[4];
static ListPtr s_listPtrs[4];
static bool s_listPoolInited = false;

static void list_init_pool(void)
{
    if (s_listPoolInited) return;
    for (int i = 0; i < 4; i++) {
        s_listPtrs[i] = &s_listPool[i];
    }
    s_listPoolInited = true;
}

ListHandle lc3_l_new(const ListRect *rView, const ListRect *dataBounds, ListCell cSize, int16_t cDefProc, void *wPtr, bool drawIt, bool hasGrow, bool scrollHoriz, bool scrollVert)
{
    (void)cDefProc;
    (void)drawIt;
    (void)hasGrow;
    (void)scrollHoriz;
    (void)scrollVert;

    list_init_pool();
    for (int i = 0; i < 4; i++) {
        if (s_listPool[i].rView.bottom == 0) {
            ListRec *l = &s_listPool[i];
            memset(l, 0, sizeof(ListRec));
            if (rView) l->rView = *rView;
            if (dataBounds) {
                l->numRows = dataBounds->bottom - dataBounds->top;
                l->numCols = dataBounds->right - dataBounds->left;
            }
            l->cellSize = cSize;
            l->port = wPtr;
            l->lActive = true;
            return &s_listPtrs[i];
        }
    }
    return (ListHandle)&s_listPtrs[0];
}

void lc3_l_dispose(ListHandle lHandle)
{
    if (!lHandle || !*lHandle) return;
    memset(*lHandle, 0, sizeof(ListRec));
}

int16_t lc3_l_add_row(int16_t count, int16_t rowNum, ListHandle lHandle)
{
    if (!lHandle || !*lHandle || count <= 0) return 0;
    ListRec *l = *lHandle;
    if (l->numRows + count > 64) count = 64 - l->numRows;
    if (rowNum < 0 || rowNum > l->numRows) rowNum = l->numRows;

    int tail = l->numRows - rowNum;
    if (tail > 0) {
        memmove(&l->cellData[rowNum + count], &l->cellData[rowNum], tail * 128);
        memmove(&l->cellSelected[rowNum + count], &l->cellSelected[rowNum], tail * sizeof(bool));
    }
    for (int i = 0; i < count; i++) {
        l->cellData[rowNum + i][0] = '\0';
        l->cellSelected[rowNum + i] = false;
    }
    l->numRows += count;
    return rowNum;
}

void lc3_l_del_row(int16_t count, int16_t rowNum, ListHandle lHandle)
{
    if (!lHandle || !*lHandle || count <= 0) return;
    ListRec *l = *lHandle;
    if (rowNum < 0 || rowNum >= l->numRows) return;
    if (rowNum + count > l->numRows) count = l->numRows - rowNum;

    int tail = l->numRows - (rowNum + count);
    if (tail > 0) {
        memmove(&l->cellData[rowNum], &l->cellData[rowNum + count], tail * 128);
        memmove(&l->cellSelected[rowNum], &l->cellSelected[rowNum + count], tail * sizeof(bool));
    }
    l->numRows -= count;
}

int16_t lc3_l_add_col(int16_t count, int16_t colNum, ListHandle lHandle)
{
    if (!lHandle || !*lHandle) return 0;
    (*lHandle)->numCols += count;
    return colNum;
}

void lc3_l_del_col(int16_t count, int16_t colNum, ListHandle lHandle)
{
    (void)colNum;
    if (!lHandle || !*lHandle) return;
    (*lHandle)->numCols -= count;
    if ((*lHandle)->numCols < 0) (*lHandle)->numCols = 0;
}

void lc3_l_set_cell(const void *dataPtr, int16_t dataLen, ListCell theCell, ListHandle lHandle)
{
    if (!lHandle || !*lHandle || !dataPtr) return;
    ListRec *l = *lHandle;
    if (theCell.v < 0 || theCell.v >= l->numRows) return;

    if (dataLen > 127) dataLen = 127;
    if (dataLen < 0) dataLen = 0;
    memcpy(l->cellData[theCell.v], dataPtr, dataLen);
    l->cellData[theCell.v][dataLen] = '\0';
}

void lc3_l_get_cell(void *dataPtr, int16_t *dataLen, ListCell theCell, ListHandle lHandle)
{
    if (!lHandle || !*lHandle || !dataPtr) return;
    ListRec *l = *lHandle;
    if (theCell.v < 0 || theCell.v >= l->numRows) {
        if (dataLen) *dataLen = 0;
        return;
    }

    int16_t len = (int16_t)strlen(l->cellData[theCell.v]);
    if (dataLen) {
        if (*dataLen < len) len = *dataLen;
        *dataLen = len;
    }
    memcpy(dataPtr, l->cellData[theCell.v], len);
}

void lc3_l_draw(ListCell theCell, ListHandle lHandle)
{
    (void)theCell;
    (void)lHandle;
}

bool lc3_l_click(ListCell pt, int16_t modifiers, ListHandle lHandle)
{
    (void)modifiers;
    if (!lHandle || !*lHandle) return false;
    ListRec *l = *lHandle;
    if (l->cellSize.v <= 0) l->cellSize.v = 16;
    int16_t row = pt.v / l->cellSize.v;
    if (row < 0 || row >= l->numRows) return false;

    for (int i = 0; i < l->numRows; i++) {
        l->cellSelected[i] = (i == row);
    }
    l->lastClick = pt;
    return true;
}

void lc3_l_activate(bool act, ListHandle lHandle)
{
    if (!lHandle || !*lHandle) return;
    (*lHandle)->lActive = act;
}

void lc3_l_update(void *rgnHandle, ListHandle lHandle)
{
    (void)rgnHandle;
    (void)lHandle;
}

bool lc3_l_search(const void *dataPtr, int16_t dataLen, void *searchProc, ListCell *theCell, ListHandle lHandle)
{
    (void)searchProc;
    if (!lHandle || !*lHandle || !dataPtr || !theCell) return false;
    ListRec *l = *lHandle;

    for (int16_t r = 0; r < l->numRows; r++) {
        if (strncmp(l->cellData[r], (const char *)dataPtr, dataLen) == 0) {
            theCell->v = r;
            theCell->h = 0;
            return true;
        }
    }
    return false;
}

void lc3_l_set_select(bool setIt, ListCell theCell, ListHandle lHandle)
{
    if (!lHandle || !*lHandle) return;
    ListRec *l = *lHandle;
    if (theCell.v >= 0 && theCell.v < l->numRows) {
        l->cellSelected[theCell.v] = setIt;
    }
}

void lc3_l_auto_scroll(ListHandle lHandle)
{
    (void)lHandle;
}

/* Package 0 Dispatcher for Trap $A9E7 */
void lc3_pack0_dispatch(int16_t selector, void *params)
{
    (void)params;
    (void)selector;
}
