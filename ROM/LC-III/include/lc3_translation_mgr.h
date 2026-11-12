/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Translation Manager (Package 9 in ROM)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_TRANSLATION_MGR_H
#define LC3_TRANSLATION_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define TR_NO_ERR            0
#define TR_NOT_POSSIBLE     -1
#define TR_BAD_SRC_ERR      -2

typedef uint32_t OSType;

typedef struct {
    int16_t vRefNum;
    int32_t parID;
    uint8_t name[64];
} TrFSSpec;

typedef struct {
    OSType formatType;
    int32_t formatFlags;
    char formatName[64];
} TranslationSpec;

/* Translation Manager API */
int16_t lc3_can_doc_be_opened(const TrFSSpec *targetDocument, int16_t openDocMode, bool *canOpen);
int16_t lc3_get_file_translation_list(const TrFSSpec *sourceDocument, void *translationListHandle);
int16_t lc3_translate_file(const TrFSSpec *sourceDocument, const TrFSSpec *destinationDocument, int16_t howToTranslate);
int16_t lc3_get_scrap_translation_list(void *translationListHandle);
int16_t lc3_translate_scrap(void *sourceData, OSType srcType, void *destData, OSType dstType);

/* Package 9 Dispatcher */
int16_t lc3_pack9_dispatch(int16_t selector, void *params);

#endif /* LC3_TRANSLATION_MGR_H */
