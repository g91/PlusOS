/* lc3_control_mgr.h - Macintosh LC III Toolbox Control Manager
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x015CD0 - 0x01641E
 */
#ifndef LC3_CONTROL_MGR_H
#define LC3_CONTROL_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define CTRL_PUSH_BUTTON     0
#define CTRL_CHECK_BOX       1
#define CTRL_RADIO_BUTTON    2
#define CTRL_SCROLL_BAR      16

#define CTRL_PART_IN_BUTTON  10
#define CTRL_PART_IN_CHECK   11
#define CTRL_PART_UP_ARROW   20
#define CTRL_PART_DOWN_ARROW 21
#define CTRL_PART_PAGE_UP    22
#define CTRL_PART_PAGE_DOWN  23
#define CTRL_PART_THUMB      129

/* Control Record */
typedef struct LC3ControlRecord {
    struct LC3ControlRecord **nextControl;
    void                     *contrlOwner;      /* Window pointer */
    int16_t                   contrlRectTop;
    int16_t                   contrlRectLeft;
    int16_t                   contrlRectBottom;
    int16_t                   contrlRectRight;
    uint8_t                   contrlVis;        /* 255 = visible, 0 = hidden */
    uint8_t                   contrlHilite;     /* 0 = active, 255 = inactive */
    int16_t                   contrlValue;      /* Current value */
    int16_t                   contrlMin;        /* Minimum value */
    int16_t                   contrlMax;        /* Maximum value */
    void                    **contrlDefProc;    /* CDEF handle */
    void                    **contrlData;       /* Private storage */
    void                    (*contrlAction)(struct LC3ControlRecord **c, int16_t part);
    int32_t                   contrlRfCon;      /* Reference constant */
    char                      contrlTitle[32];  /* Pascal title */
} LC3ControlRecord;

typedef LC3ControlRecord **LC3ControlHandle;

/* Control Manager API */
LC3ControlHandle lc3_ctrl_new(void *window, int16_t top, int16_t left, int16_t bottom, int16_t right,
                              const char *title, bool visible, int16_t value, int16_t min, int16_t max,
                              int16_t procID, int32_t refCon);
void             lc3_ctrl_dispose(LC3ControlHandle c);
void             lc3_ctrl_show(LC3ControlHandle c);
void             lc3_ctrl_hide(LC3ControlHandle c);
void             lc3_ctrl_move(LC3ControlHandle c, int16_t h, int16_t v);
void             lc3_ctrl_draw_all(void *window);
void             lc3_ctrl_draw_one(LC3ControlHandle c);
void             lc3_ctrl_hilite(LC3ControlHandle c, uint8_t hiliteCode);
int16_t          lc3_ctrl_test(LC3ControlHandle c, int16_t h, int16_t v);
int16_t          lc3_ctrl_find(int16_t h, int16_t v, void *window, LC3ControlHandle *whichCtrl);
int16_t          lc3_ctrl_track(LC3ControlHandle c, int16_t startH, int16_t startV, void (*action)(LC3ControlHandle, int16_t));
void             lc3_ctrl_set_value(LC3ControlHandle c, int16_t value);
int16_t          lc3_ctrl_get_value(LC3ControlHandle c);
void             lc3_ctrl_set_min(LC3ControlHandle c, int16_t min);
void             lc3_ctrl_set_max(LC3ControlHandle c, int16_t max);

#endif /* LC3_CONTROL_MGR_H */
