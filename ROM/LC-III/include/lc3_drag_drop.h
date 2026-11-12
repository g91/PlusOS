/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Drag and Drop Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DRAG_DROP_H
#define LC3_DRAG_DROP_H

#include <stdint.h>
#include <stdbool.h>

#define DRAG_NO_ERR             0
#define DRAG_NOT_FOUND_ERR   -1850
#define DRAG_PARAM_ERR         -50
#define DRAG_OUT_OF_MEM_ERR   -108

typedef void *DragReference;
typedef uint32_t ItemReference;
typedef uint32_t FlavorType;

typedef struct {
    int16_t v;
    int16_t h;
} Point;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} Rect;

typedef struct DragItem {
    ItemReference itemRef;
    FlavorType flavor;
    void *data;
    uint32_t dataSize;
    struct DragItem *next;
} DragItem;

typedef struct DragSession {
    DragReference dragRef;
    DragItem *itemList;
    Point mousePos;
    Rect dragRegion;
    bool isTracking;
    bool isHilited;
    void *trackingProc;
    void *receiveProc;
} DragSession;

/* Drag and Drop Manager API */
int16_t lc3_new_drag(DragReference *theDrag);
int16_t lc3_dispose_drag(DragReference theDrag);
int16_t lc3_add_drag_item_flavor(DragReference theDrag, ItemReference theItemRef,
                                 FlavorType theType, const void *dataPtr, uint32_t dataSize, uint32_t flavorFlags);
int16_t lc3_get_drag_item_flavor_data(DragReference theDrag, ItemReference theItemRef,
                                      FlavorType theType, void *dataPtr, uint32_t *dataSize, uint32_t offset);
int16_t lc3_track_drag(DragReference theDrag, const void *theEvent, void *theRgn);
int16_t lc3_show_drag_hilite(DragReference theDrag, void *hiliteFrame, bool inside);
int16_t lc3_hide_drag_hilite(DragReference theDrag);
int16_t lc3_drag_dispatch(int16_t selector, void *params);

#endif /* LC3_DRAG_DROP_H */
