/* lc3_event_queue.h - Macintosh LC III OS Event Manager & Queue Core
 * Fully reverse-engineered from Universal ROM $067C (lc3_rom.map)
 * Offsets 0x009966 - 0x00B508
 */
#ifndef LC3_EVENT_QUEUE_H
#define LC3_EVENT_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#define EVT_NULL             0
#define EVT_MOUSEDOWN        1
#define EVT_MOUSEUP          2
#define EVT_KEYDOWN          3
#define EVT_KEYUP            4
#define EVT_AUTO_KEY         5
#define EVT_UPDATE           6
#define EVT_DISK             7
#define EVT_ACTIVATE         8
#define EVT_OS               15

/* Mac OS Event Record */
typedef struct LC3EventRecord {
    uint16_t what;              /* Event type */
    uint32_t message;           /* Character code, window ptr, or drive num */
    uint32_t when;              /* Ticks timestamp */
    int16_t  whereV;            /* Mouse vertical coordinate */
    int16_t  whereH;            /* Mouse horizontal coordinate */
    uint16_t modifiers;         /* Shift, Option, Command, CapsLock */
} LC3EventRecord;

/* Event Queue Element */
typedef struct LC3EvtQEl {
    struct LC3EvtQEl *qLink;
    uint16_t          qType;
    LC3EventRecord    evt;
} LC3EvtQEl;

/* Queue Header Record */
typedef struct {
    uint16_t   qFlags;
    LC3EvtQEl *qHead;
    LC3EvtQEl *qTail;
} LC3QueueHeader;

/* Event Manager Low-Memory Globals */
#define LM_EVENTQUEUE        (*(volatile LC3QueueHeader *)0x014A)
#define LM_MOUSEPOSV         (*(volatile int16_t *)0x0828)
#define LM_MOUSEPOSH         (*(volatile int16_t *)0x082A)

/* Event Manager API */
void     lc3_event_init_queue(void);
int16_t  lc3_post_event(uint16_t eventNum, uint32_t eventMsg);
bool     lc3_os_event_avail(uint16_t eventMask, LC3EventRecord *theEvent);
bool     lc3_get_os_event(uint16_t eventMask, LC3EventRecord *theEvent);
void     lc3_flush_events(uint16_t whichMask, uint16_t stopMask);

#endif /* LC3_EVENT_QUEUE_H */
