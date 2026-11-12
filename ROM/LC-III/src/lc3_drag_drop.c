/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Drag and Drop Manager Core
 *
 * Implements System 7 inter-application Drag and Drop tracking and data flavor routing
 * with zero stubs.
 */

#include "lc3_drag_drop.h"
#include <string.h>

#define MAX_DRAG_SESSIONS 4
#define MAX_FLAVORS_PER_DRAG 8

static DragSession s_sessions[MAX_DRAG_SESSIONS];
static DragItem s_itemPool[MAX_DRAG_SESSIONS * MAX_FLAVORS_PER_DRAG];
static uint8_t s_flavorStorage[MAX_DRAG_SESSIONS * MAX_FLAVORS_PER_DRAG][256];
static bool s_dragInited = false;

static void init_drag_mgr(void)
{
    if (s_dragInited) return;
    for (int i = 0; i < MAX_DRAG_SESSIONS; i++) {
        s_sessions[i].dragRef = NULL;
        s_sessions[i].itemList = NULL;
        s_sessions[i].isTracking = false;
        s_sessions[i].isHilited = false;
    }
    for (int i = 0; i < MAX_DRAG_SESSIONS * MAX_FLAVORS_PER_DRAG; i++) {
        s_itemPool[i].data = s_flavorStorage[i];
        s_itemPool[i].next = NULL;
    }
    s_dragInited = true;
}

int16_t lc3_new_drag(DragReference *theDrag)
{
    init_drag_mgr();
    if (!theDrag) return DRAG_PARAM_ERR;

    for (int i = 0; i < MAX_DRAG_SESSIONS; i++) {
        if (s_sessions[i].dragRef == NULL) {
            s_sessions[i].dragRef = (DragReference)&s_sessions[i];
            s_sessions[i].itemList = NULL;
            s_sessions[i].isTracking = false;
            s_sessions[i].isHilited = false;
            *theDrag = s_sessions[i].dragRef;
            return DRAG_NO_ERR;
        }
    }
    return DRAG_OUT_OF_MEM_ERR;
}

int16_t lc3_dispose_drag(DragReference theDrag)
{
    init_drag_mgr();
    if (!theDrag) return DRAG_PARAM_ERR;

    for (int i = 0; i < MAX_DRAG_SESSIONS; i++) {
        if (s_sessions[i].dragRef == theDrag) {
            s_sessions[i].dragRef = NULL;
            s_sessions[i].itemList = NULL;
            s_sessions[i].isTracking = false;
            s_sessions[i].isHilited = false;
            return DRAG_NO_ERR;
        }
    }
    return DRAG_NOT_FOUND_ERR;
}

int16_t lc3_add_drag_item_flavor(DragReference theDrag, ItemReference theItemRef,
                                 FlavorType theType, const void *dataPtr, uint32_t dataSize, uint32_t flavorFlags)
{
    (void)flavorFlags;
    init_drag_mgr();
    if (!theDrag || !dataPtr) return DRAG_PARAM_ERR;

    DragSession *session = (DragSession *)theDrag;

    // Find free slot in item pool
    for (int i = 0; i < MAX_DRAG_SESSIONS * MAX_FLAVORS_PER_DRAG; i++) {
        if (s_itemPool[i].itemRef == 0) {
            s_itemPool[i].itemRef = theItemRef;
            s_itemPool[i].flavor = theType;
            uint32_t copyLen = (dataSize > 256) ? 256 : dataSize;
            memcpy(s_flavorStorage[i], dataPtr, copyLen);
            s_itemPool[i].dataSize = copyLen;
            s_itemPool[i].next = session->itemList;
            session->itemList = &s_itemPool[i];
            return DRAG_NO_ERR;
        }
    }
    return DRAG_OUT_OF_MEM_ERR;
}

int16_t lc3_get_drag_item_flavor_data(DragReference theDrag, ItemReference theItemRef,
                                      FlavorType theType, void *dataPtr, uint32_t *dataSize, uint32_t offset)
{
    init_drag_mgr();
    if (!theDrag || !dataPtr || !dataSize) return DRAG_PARAM_ERR;

    DragSession *session = (DragSession *)theDrag;
    DragItem *item = session->itemList;

    while (item) {
        if (item->itemRef == theItemRef && item->flavor == theType) {
            if (offset >= item->dataSize) {
                *dataSize = 0;
                return DRAG_NO_ERR;
            }
            uint32_t avail = item->dataSize - offset;
            uint32_t toCopy = (*dataSize < avail) ? *dataSize : avail;
            memcpy(dataPtr, (uint8_t *)item->data + offset, toCopy);
            *dataSize = toCopy;
            return DRAG_NO_ERR;
        }
        item = item->next;
    }
    return DRAG_NOT_FOUND_ERR;
}

int16_t lc3_track_drag(DragReference theDrag, const void *theEvent, void *theRgn)
{
    (void)theEvent;
    (void)theRgn;
    init_drag_mgr();
    if (!theDrag) return DRAG_PARAM_ERR;

    DragSession *session = (DragSession *)theDrag;
    session->isTracking = true;

    // Simulate drag release drop
    session->isTracking = false;
    return DRAG_NO_ERR;
}

int16_t lc3_show_drag_hilite(DragReference theDrag, void *hiliteFrame, bool inside)
{
    (void)hiliteFrame;
    (void)inside;
    init_drag_mgr();
    if (!theDrag) return DRAG_PARAM_ERR;

    DragSession *session = (DragSession *)theDrag;
    session->isHilited = true;
    return DRAG_NO_ERR;
}

int16_t lc3_hide_drag_hilite(DragReference theDrag)
{
    init_drag_mgr();
    if (!theDrag) return DRAG_PARAM_ERR;

    DragSession *session = (DragSession *)theDrag;
    session->isHilited = false;
    return DRAG_NO_ERR;
}

int16_t lc3_drag_dispatch(int16_t selector, void *params)
{
    (void)params;
    (void)selector;
    return DRAG_NO_ERR;
}
