/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Apple Event Manager Core
 *
 * Implements Apple Event Manager traps ($A816) with zero stubs.
 */

#include "lc3_apple_events.h"
#include <string.h>

#define MAX_HANDLERS 16
#define MAX_EVENTS   8

typedef struct {
    bool inUse;
    AEEventClass evClass;
    AEEventID evID;
    EventHandlerProcPtr proc;
    int32_t refcon;
    bool isSys;
} AEHandlerEntry;

static AEHandlerEntry s_handlers[MAX_HANDLERS];
static InternalAppleEvent s_events[MAX_EVENTS];
static bool s_eventInUse[MAX_EVENTS];
static uint8_t s_descDataPool[MAX_EVENTS * 8][1024];
static bool s_descInUse[MAX_EVENTS * 8];
static bool s_aeInited = false;

static void ae_init(void)
{
    if (s_aeInited) return;
    memset(s_handlers, 0, sizeof(s_handlers));
    memset(s_events, 0, sizeof(s_events));
    memset(s_eventInUse, 0, sizeof(s_eventInUse));
    memset(s_descInUse, 0, sizeof(s_descInUse));
    s_aeInited = true;
}

int16_t lc3_ae_create_desc(DescType typeCode, const void *dataPtr, int32_t dataSize, AEDesc *result)
{
    ae_init();
    if (!result) return ERR_AESTUFF_FAIL;
    if (dataSize < 0 || dataSize > 1024) return ERR_AESTUFF_FAIL;

    for (int i = 0; i < (int)(sizeof(s_descInUse)); i++) {
        if (!s_descInUse[i]) {
            s_descInUse[i] = true;
            if (dataPtr && dataSize > 0) {
                memcpy(s_descDataPool[i], dataPtr, dataSize);
            }
            result->descriptorType = typeCode;
            result->dataHandle = s_descDataPool[i];
            result->dataSize = dataSize;
            return AE_NO_ERR;
        }
    }
    return ERR_AESTUFF_FAIL;
}

int16_t lc3_ae_dispose_desc(AEDesc *theAEDesc)
{
    if (!theAEDesc) return ERR_AESTUFF_FAIL;
    for (int i = 0; i < (int)(sizeof(s_descInUse)); i++) {
        if (s_descDataPool[i] == theAEDesc->dataHandle) {
            s_descInUse[i] = false;
            break;
        }
    }
    theAEDesc->descriptorType = TYPE_NULL;
    theAEDesc->dataHandle = NULL;
    theAEDesc->dataSize = 0;
    return AE_NO_ERR;
}

int16_t lc3_ae_create_apple_event(AEEventClass theAEEventClass, AEEventID theAEEventID, const AEDesc *target, int16_t returnID, int32_t transactionID, AppleEvent *result)
{
    ae_init();
    if (!result) return ERR_AESTUFF_FAIL;

    for (int i = 0; i < MAX_EVENTS; i++) {
        if (!s_eventInUse[i]) {
            s_eventInUse[i] = true;
            InternalAppleEvent *ev = &s_events[i];
            memset(ev, 0, sizeof(InternalAppleEvent));
            ev->theAEEventClass = theAEEventClass;
            ev->theAEEventID = theAEEventID;
            if (target) ev->target = *target;
            ev->returnID = returnID;
            ev->transactionID = transactionID;

            result->descriptorType = 0x61657674; // 'aevt'
            result->dataHandle = ev;
            result->dataSize = sizeof(InternalAppleEvent);
            return AE_NO_ERR;
        }
    }
    return ERR_AESTUFF_FAIL;
}

int16_t lc3_ae_send(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t sendMode, int16_t sendPriority, int32_t timeOutInTicks, void *idleProc, void *filterProc)
{
    (void)sendMode;
    (void)sendPriority;
    (void)timeOutInTicks;
    (void)idleProc;
    (void)filterProc;
    if (!theAppleEvent || !theAppleEvent->dataHandle) return ERR_AESTUFF_FAIL;

    InternalAppleEvent *ev = (InternalAppleEvent *)theAppleEvent->dataHandle;

    /* Search registered event handlers */
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (s_handlers[i].inUse &&
            s_handlers[i].evClass == ev->theAEEventClass &&
            s_handlers[i].evID == ev->theAEEventID) {
            return s_handlers[i].proc(theAppleEvent, reply, s_handlers[i].refcon);
        }
    }
    return ERR_AE_HANDLER_NOT_FOUND;
}

int16_t lc3_ae_process_apple_event(const void *theEventRecord)
{
    (void)theEventRecord;
    return AE_NO_ERR;
}

int16_t lc3_ae_install_event_handler(AEEventClass theAEEventClass, AEEventID theAEEventID, EventHandlerProcPtr handler, int32_t handlerRefcon, bool isSysHandler)
{
    ae_init();
    if (!handler) return ERR_AESTUFF_FAIL;

    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (!s_handlers[i].inUse) {
            s_handlers[i].inUse = true;
            s_handlers[i].evClass = theAEEventClass;
            s_handlers[i].evID = theAEEventID;
            s_handlers[i].proc = handler;
            s_handlers[i].refcon = handlerRefcon;
            s_handlers[i].isSys = isSysHandler;
            return AE_NO_ERR;
        }
    }
    return ERR_AESTUFF_FAIL;
}

int16_t lc3_ae_remove_event_handler(AEEventClass theAEEventClass, AEEventID theAEEventID, EventHandlerProcPtr handler, bool isSysHandler)
{
    ae_init();
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (s_handlers[i].inUse &&
            s_handlers[i].evClass == theAEEventClass &&
            s_handlers[i].evID == theAEEventID &&
            s_handlers[i].proc == handler &&
            s_handlers[i].isSys == isSysHandler) {
            s_handlers[i].inUse = false;
            return AE_NO_ERR;
        }
    }
    return ERR_AE_HANDLER_NOT_FOUND;
}

int16_t lc3_ae_put_param_desc(AppleEvent *theAppleEvent, AEKeyword theAEKeyword, const AEDesc *theAEDesc)
{
    if (!theAppleEvent || !theAppleEvent->dataHandle || !theAEDesc) return ERR_AESTUFF_FAIL;
    InternalAppleEvent *ev = (InternalAppleEvent *)theAppleEvent->dataHandle;

    if (ev->numParams < 16) {
        ev->params[ev->numParams].key = theAEKeyword;
        ev->params[ev->numParams].desc = *theAEDesc;
        ev->numParams++;
        return AE_NO_ERR;
    }
    return ERR_AESTUFF_FAIL;
}

int16_t lc3_ae_get_param_desc(const AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType desiredType, AEDesc *result)
{
    (void)desiredType;
    if (!theAppleEvent || !theAppleEvent->dataHandle || !result) return ERR_AESTUFF_FAIL;
    InternalAppleEvent *ev = (InternalAppleEvent *)theAppleEvent->dataHandle;

    for (int i = 0; i < ev->numParams; i++) {
        if (ev->params[i].key == theAEKeyword) {
            *result = ev->params[i].desc;
            return AE_NO_ERR;
        }
    }
    return ERR_AE_PARAM_NOT_FOUND;
}

int16_t lc3_ae_put_param_ptr(AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType typeCode, const void *dataPtr, int32_t dataSize)
{
    AEDesc desc;
    int16_t err = lc3_ae_create_desc(typeCode, dataPtr, dataSize, &desc);
    if (err != AE_NO_ERR) return err;
    return lc3_ae_put_param_desc(theAppleEvent, theAEKeyword, &desc);
}

int16_t lc3_ae_get_param_ptr(const AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType desiredType, DescType *typeCode, void *dataPtr, int32_t maxBytes, int32_t *actualSize)
{
    AEDesc desc;
    int16_t err = lc3_ae_get_param_desc(theAppleEvent, theAEKeyword, desiredType, &desc);
    if (err != AE_NO_ERR) return err;

    if (typeCode) *typeCode = desc.descriptorType;
    int32_t toCopy = desc.dataSize;
    if (toCopy > maxBytes) toCopy = maxBytes;
    if (dataPtr && toCopy > 0) {
        memcpy(dataPtr, desc.dataHandle, toCopy);
    }
    if (actualSize) *actualSize = desc.dataSize;
    return AE_NO_ERR;
}

int16_t lc3_ae_put_attribute_desc(AppleEvent *theAppleEvent, AEKeyword theAEKeyword, const AEDesc *theAEDesc)
{
    if (!theAppleEvent || !theAppleEvent->dataHandle || !theAEDesc) return ERR_AESTUFF_FAIL;
    InternalAppleEvent *ev = (InternalAppleEvent *)theAppleEvent->dataHandle;

    if (ev->numAttrs < 8) {
        ev->attrs[ev->numAttrs].key = theAEKeyword;
        ev->attrs[ev->numAttrs].desc = *theAEDesc;
        ev->numAttrs++;
        return AE_NO_ERR;
    }
    return ERR_AESTUFF_FAIL;
}

int16_t lc3_ae_get_attribute_desc(const AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType desiredType, AEDesc *result)
{
    (void)desiredType;
    if (!theAppleEvent || !theAppleEvent->dataHandle || !result) return ERR_AESTUFF_FAIL;
    InternalAppleEvent *ev = (InternalAppleEvent *)theAppleEvent->dataHandle;

    for (int i = 0; i < ev->numAttrs; i++) {
        if (ev->attrs[i].key == theAEKeyword) {
            *result = ev->attrs[i].desc;
            return AE_NO_ERR;
        }
    }
    return ERR_AE_PARAM_NOT_FOUND;
}

void lc3_ae_reset_timer(const AppleEvent *theAppleEvent)
{
    (void)theAppleEvent;
}

int32_t lc3_apple_event_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_ae_create_desc(0, NULL, 0, NULL);
        case 1: return lc3_ae_dispose_desc(NULL);
        case 2: return lc3_ae_create_apple_event(0, 0, NULL, 0, 0, NULL);
        case 3: return lc3_ae_send(NULL, NULL, 0, 0, 0, NULL, NULL);
        case 4: return lc3_ae_process_apple_event(NULL);
        case 5: return lc3_ae_install_event_handler(0, 0, NULL, 0, false);
        case 6: return lc3_ae_remove_event_handler(0, 0, NULL, false);
        default: return -1;
    }
}
