/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Text Services Manager (TSM Core)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TEXT_SERVICES_H
#define LC3_TEXT_SERVICES_H

#include <stdint.h>
#include <stdbool.h>

#define TSM_NO_ERR           0
#define TSM_DOC_NOT_FOUND   -1
#define TSM_PARAM_ERR      -50

typedef void *TSMDocumentID;
typedef uint32_t OSType;

typedef struct {
    bool active;
    int16_t numOfInterface;
    OSType supportedInterface[4];
    int32_t refCon;
} TSMDocRecord;

/* Text Services Manager API */
int16_t lc3_new_tsm_document(int16_t numOfInterface, const OSType *supportedInterfaceTypes, TSMDocumentID *idocID, int32_t refCon);
int16_t lc3_delete_tsm_document(TSMDocumentID idocID);
int16_t lc3_activate_tsm_document(TSMDocumentID idocID);
int16_t lc3_deactivate_tsm_document(TSMDocumentID idocID);
int16_t lc3_fix_tsm_document(TSMDocumentID idocID);
int16_t lc3_set_tsm_document_property(TSMDocumentID idocID, OSType propertyType, int32_t propertyData);
int16_t lc3_get_tsm_document_property(TSMDocumentID idocID, OSType propertyType, int32_t *propertyData);
int16_t lc3_use_input_window(TSMDocumentID idocID, bool useWindow);
bool lc3_tsm_event(void *event);

/* Text Services Dispatch ($AA54) */
int32_t lc3_tsm_dispatch(int16_t selector, void *params);

#endif /* LC3_TEXT_SERVICES_H */
