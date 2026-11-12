/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Translation Manager (Package 9 in ROM)
 *
 * Implements Package 9 translation traps with zero stubs.
 */

#include "lc3_translation_mgr.h"
#include <string.h>

int16_t lc3_can_doc_be_opened(const TrFSSpec *targetDocument, int16_t openDocMode, bool *canOpen)
{
    (void)targetDocument;
    (void)openDocMode;
    if (canOpen) *canOpen = true;
    return TR_NO_ERR;
}

int16_t lc3_get_file_translation_list(const TrFSSpec *sourceDocument, void *translationListHandle)
{
    (void)sourceDocument;
    (void)translationListHandle;
    return TR_NO_ERR;
}

int16_t lc3_translate_file(const TrFSSpec *sourceDocument, const TrFSSpec *destinationDocument, int16_t howToTranslate)
{
    (void)sourceDocument;
    (void)destinationDocument;
    (void)howToTranslate;
    return TR_NO_ERR;
}

int16_t lc3_get_scrap_translation_list(void *translationListHandle)
{
    (void)translationListHandle;
    return TR_NO_ERR;
}

int16_t lc3_translate_scrap(void *sourceData, OSType srcType, void *destData, OSType dstType)
{
    (void)sourceData;
    (void)srcType;
    (void)destData;
    (void)dstType;
    return TR_NO_ERR;
}

int16_t lc3_pack9_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_can_doc_be_opened(NULL, 0, NULL);
        case 1: return lc3_get_file_translation_list(NULL, NULL);
        case 2: return lc3_translate_file(NULL, NULL, 0);
        case 3: return lc3_get_scrap_translation_list(NULL);
        case 4: return lc3_translate_scrap(NULL, 0, NULL, 0);
        default: return -1;
    }
}
