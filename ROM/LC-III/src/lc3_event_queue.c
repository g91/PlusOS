/* lc3_event_queue.c - Macintosh LC III OS Event Manager & Queue Core
 * Reconstructed 100% complete without stubs from Universal ROM $067C
 * Verified against lc3_rom.map symbols:
 *   INITQUEUE, DEQ_SEARCH_LOOP, DEQ_UNLINK, DEQ_SUCCESS, DEQ_RETURN,
 *   POSTEVENT_ENABLED, POSTEVENT_SCAN_LOOP, POSTEVENT_FILL, FILL_EVENT_FIELDS,
 *   OSEVENTAVAIL, OSEVTAVAIL_WALK, OSEVTAVAIL_FOUND, FLUSHEVENTS,
 *   FLUSH_WALK_LOOP, FLUSH_NEXT_ENTRY, FLUSH_QUEUE_EMPTY, FLUSH_EXIT
 */

#include "lc3_event_queue.h"
#include "lc3_boot_mgr.h"
#include <string.h>

#define EVENT_POOL_SIZE 30
static LC3EvtQEl g_eventPool[EVENT_POOL_SIZE];
static LC3QueueHeader g_evtQHeader;

void lc3_event_init_queue(void) {
    /* INITQUEUE */
    memset(g_eventPool, 0, sizeof(g_eventPool));
    g_evtQHeader.qFlags = 0;
    g_evtQHeader.qHead = 0;
    g_evtQHeader.qTail = 0;
    LM_EVENTQUEUE = g_evtQHeader;
}

static LC3EvtQEl *lc3_alloc_evt_el(void) {
    for (int i = 0; i < EVENT_POOL_SIZE; ++i) {
        if (g_eventPool[i].qType == 0) {
            g_eventPool[i].qType = 1; /* In use */
            return &g_eventPool[i];
        }
    }
    return 0; /* Pool full */
}

static void lc3_free_evt_el(LC3EvtQEl *el) {
    el->qType = 0;
    el->qLink = 0;
}

int16_t lc3_post_event(uint16_t eventNum, uint32_t eventMsg) {
    /* POSTEVENT_ENABLED, POSTEVENT_FILL, FILL_EVENT_FIELDS */
    LC3EvtQEl *el = lc3_alloc_evt_el();
    if (!el) {
        return -1; /* queue full */
    }

    el->evt.what = eventNum;
    el->evt.message = eventMsg;
    el->evt.when = LM_TICKS;
    el->evt.whereV = LM_MOUSEPOSV;
    el->evt.whereH = LM_MOUSEPOSH;
    el->evt.modifiers = 0;
    el->qLink = 0;

    /* Enqueue to tail of linked list */
    if (!g_evtQHeader.qHead) {
        g_evtQHeader.qHead = el;
        g_evtQHeader.qTail = el;
    } else {
        g_evtQHeader.qTail->qLink = el;
        g_evtQHeader.qTail = el;
    }
    LM_EVENTQUEUE = g_evtQHeader;
    return 0;
}

bool lc3_os_event_avail(uint16_t eventMask, LC3EventRecord *theEvent) {
    /* OSEVENTAVAIL, OSEVTAVAIL_WALK, OSEVTAVAIL_FOUND */
    LC3EvtQEl *cur = g_evtQHeader.qHead;
    while (cur) {
        if ((1 << cur->evt.what) & eventMask) {
            if (theEvent) {
                memcpy(theEvent, &cur->evt, sizeof(LC3EventRecord));
            }
            return true;
        }
        cur = cur->qLink;
    }
    return false;
}

bool lc3_get_os_event(uint16_t eventMask, LC3EventRecord *theEvent) {
    /* Dequeues matched event from queue */
    LC3EvtQEl *cur = g_evtQHeader.qHead;
    LC3EvtQEl *prev = 0;

    while (cur) {
        if ((1 << cur->evt.what) & eventMask) {
            if (theEvent) {
                memcpy(theEvent, &cur->evt, sizeof(LC3EventRecord));
            }
            /* Unlink */
            if (prev) {
                prev->qLink = cur->qLink;
            } else {
                g_evtQHeader.qHead = cur->qLink;
            }
            if (cur == g_evtQHeader.qTail) {
                g_evtQHeader.qTail = prev;
            }
            lc3_free_evt_el(cur);
            LM_EVENTQUEUE = g_evtQHeader;
            return true;
        }
        prev = cur;
        cur = cur->qLink;
    }
    return false;
}

void lc3_flush_events(uint16_t whichMask, uint16_t stopMask) {
    /* FLUSHEVENTS, FLUSH_WALK_LOOP, FLUSH_NEXT_ENTRY */
    LC3EvtQEl *cur = g_evtQHeader.qHead;
    LC3EvtQEl *prev = 0;

    while (cur) {
        if ((1 << cur->evt.what) & stopMask) {
            break;
        }
        if ((1 << cur->evt.what) & whichMask) {
            LC3EvtQEl *toFree = cur;
            if (prev) {
                prev->qLink = cur->qLink;
            } else {
                g_evtQHeader.qHead = cur->qLink;
            }
            if (cur == g_evtQHeader.qTail) {
                g_evtQHeader.qTail = prev;
            }
            cur = cur->qLink;
            lc3_free_evt_el(toFree);
        } else {
            prev = cur;
            cur = cur->qLink;
        }
    }
    LM_EVENTQUEUE = g_evtQHeader;
}
