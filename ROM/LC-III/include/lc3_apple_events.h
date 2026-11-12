/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: System 7 Apple Event Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLE_EVENTS_H
#define LC3_APPLE_EVENTS_H

#include <stdint.h>
#include <stdbool.h>

#define AE_NO_ERR             0
#define ERR_AESTUFF_FAIL   -1700
#define ERR_AE_CORRUPT_DATA -1702
#define ERR_AE_WRONG_TYPE  -1703
#define ERR_AE_PARAM_NOT_FOUND -1701
#define ERR_AE_HANDLER_NOT_FOUND -1717

typedef uint32_t AEEventClass;
typedef uint32_t AEEventID;
typedef uint32_t AEKeyword;
typedef uint32_t DescType;

#define TYPE_NULL          0x6E756C6C // 'null'
#define TYPE_BOOLEAN       0x626F6F6C // 'bool'
#define TYPE_SHORT_INT     0x73686F72 // 'shor'
#define TYPE_LONG_INT      0x6C6F6E67 // 'long'
#define TYPE_CHAR          0x54455854 // 'TEXT'
#define TYPE_TYPE          0x74797065 // 'type'

#define K_AE_NO_REPLY      0x00000001
#define K_AE_QUEUE_REPLY   0x00000002
#define K_AE_WAIT_REPLY    0x00000003

typedef struct {
    DescType descriptorType;
    void *dataHandle;
    uint32_t dataSize;
} AEDesc, *AEDescPtr;

typedef AEDesc AppleEvent;

typedef struct {
    AEKeyword key;
    AEDesc desc;
} AEParamEntry;

typedef struct {
    AEEventClass theAEEventClass;
    AEEventID theAEEventID;
    AEDesc target;
    int16_t returnID;
    int32_t transactionID;
    int16_t numParams;
    AEParamEntry params[16];
    int16_t numAttrs;
    AEParamEntry attrs[8];
} InternalAppleEvent;

typedef int16_t (*EventHandlerProcPtr)(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t handlerRefcon);

/* Apple Event Manager API */
int16_t lc3_ae_create_desc(DescType typeCode, const void *dataPtr, int32_t dataSize, AEDesc *result);
int16_t lc3_ae_dispose_desc(AEDesc *theAEDesc);
int16_t lc3_ae_create_apple_event(AEEventClass theAEEventClass, AEEventID theAEEventID, const AEDesc *target, int16_t returnID, int32_t transactionID, AppleEvent *result);
int16_t lc3_ae_send(const AppleEvent *theAppleEvent, AppleEvent *reply, int32_t sendMode, int16_t sendPriority, int32_t timeOutInTicks, void *idleProc, void *filterProc);
int16_t lc3_ae_process_apple_event(const void *theEventRecord);
int16_t lc3_ae_install_event_handler(AEEventClass theAEEventClass, AEEventID theAEEventID, EventHandlerProcPtr handler, int32_t handlerRefcon, bool isSysHandler);
int16_t lc3_ae_remove_event_handler(AEEventClass theAEEventClass, AEEventID theAEEventID, EventHandlerProcPtr handler, bool isSysHandler);
int16_t lc3_ae_put_param_desc(AppleEvent *theAppleEvent, AEKeyword theAEKeyword, const AEDesc *theAEDesc);
int16_t lc3_ae_get_param_desc(const AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType desiredType, AEDesc *result);
int16_t lc3_ae_put_param_ptr(AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType typeCode, const void *dataPtr, int32_t dataSize);
int16_t lc3_ae_get_param_ptr(const AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType desiredType, DescType *typeCode, void *dataPtr, int32_t maxBytes, int32_t *actualSize);
int16_t lc3_ae_put_attribute_desc(AppleEvent *theAppleEvent, AEKeyword theAEKeyword, const AEDesc *theAEDesc);
int16_t lc3_ae_get_attribute_desc(const AppleEvent *theAppleEvent, AEKeyword theAEKeyword, DescType desiredType, AEDesc *result);
void lc3_ae_reset_timer(const AppleEvent *theAppleEvent);

/* Apple Event Dispatcher ($A816) */
int32_t lc3_apple_event_dispatch(int16_t selector, void *params);

#endif /* LC3_APPLE_EVENTS_H */
