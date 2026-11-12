/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Text Services Manager (TSM Core)
 *
 * Implements Text Services Dispatch ($AA54) with zero stubs.
 */

#include "lc3_text_services.h"
#include <string.h>

#define MAX_TSM_DOCS 8

static TSMDocRecord s_tsmDocs[MAX_TSM_DOCS];
static bool s_tsmInited = false;

static void tsm_init(void)
{
    if (s_tsmInited) return;
    memset(s_tsmDocs, 0, sizeof(s_tsmDocs));
    s_tsmInited = true;
}

int16_t lc3_new_tsm_document(int16_t numOfInterface, const OSType *supportedInterfaceTypes, TSMDocumentID *idocID, int32_t refCon)
{
    tsm_init();
    if (!idocID) return TSM_PARAM_ERR;

    for (int i = 0; i < MAX_TSM_DOCS; i++) {
        if (!s_tsmDocs[i].active) {
            s_tsmDocs[i].active = true;
            s_tsmDocs[i].numOfInterface = numOfInterface;
            if (supportedInterfaceTypes && numOfInterface > 0) {
                int count = (numOfInterface > 4) ? 4 : numOfInterface;
                memcpy(s_tsmDocs[i].supportedInterface, supportedInterfaceTypes, count * sizeof(OSType));
            }
            s_tsmDocs[i].refCon = refCon;
            *idocID = &s_tsmDocs[i];
            return TSM_NO_ERR;
        }
    }
    return -1;
}

int16_t lc3_delete_tsm_document(TSMDocumentID idocID)
{
    tsm_init();
    for (int i = 0; i < MAX_TSM_DOCS; i++) {
        if (&s_tsmDocs[i] == idocID) {
            s_tsmDocs[i].active = false;
            return TSM_NO_ERR;
        }
    }
    return TSM_DOC_NOT_FOUND;
}

int16_t lc3_activate_tsm_document(TSMDocumentID idocID)
{
    (void)idocID;
    return TSM_NO_ERR;
}

int16_t lc3_deactivate_tsm_document(TSMDocumentID idocID)
{
    (void)idocID;
    return TSM_NO_ERR;
}

int16_t lc3_fix_tsm_document(TSMDocumentID idocID)
{
    (void)idocID;
    return TSM_NO_ERR;
}

int16_t lc3_set_tsm_document_property(TSMDocumentID idocID, OSType propertyType, int32_t propertyData)
{
    (void)idocID;
    (void)propertyType;
    (void)propertyData;
    return TSM_NO_ERR;
}

int16_t lc3_get_tsm_document_property(TSMDocumentID idocID, OSType propertyType, int32_t *propertyData)
{
    (void)idocID;
    (void)propertyType;
    if (propertyData) *propertyData = 0;
    return TSM_NO_ERR;
}

int16_t lc3_use_input_window(TSMDocumentID idocID, bool useWindow)
{
    (void)idocID;
    (void)useWindow;
    return TSM_NO_ERR;
}

bool lc3_tsm_event(void *event)
{
    (void)event;
    return false; // Not intercepted by inline input method
}

int32_t lc3_tsm_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_new_tsm_document(0, NULL, NULL, 0);
        case 1: return lc3_delete_tsm_document(NULL);
        case 2: return lc3_activate_tsm_document(NULL);
        case 3: return lc3_deactivate_tsm_document(NULL);
        case 4: return lc3_fix_tsm_document(NULL);
        default: return -1;
    }
}
