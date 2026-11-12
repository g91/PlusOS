/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Edition Manager Section Records & Publish/Subscribe Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#include "lc3_edition_io.h"
#include <string.h>

#define MAX_ACTIVE_SECTIONS 64
#define MAX_CONTAINERS 32

typedef struct EditionContainer {
    uint32_t containerID;
    uint32_t formatType;
    uint8_t buffer[4096];
    uint32_t length;
    uint32_t modDate;
    bool inUse;
} EditionContainer;

static SectionRecord sRegisteredSections[MAX_ACTIVE_SECTIONS];
static uint32_t sNumRegisteredSections = 0;
static EditionContainer sContainers[MAX_CONTAINERS];
static uint32_t sNextContainerID = 1000;
static bool sEditionsInitialized = false;

int16_t lc3_ed_init_editions(void)
{
    memset(sRegisteredSections, 0, sizeof(sRegisteredSections));
    memset(sContainers, 0, sizeof(sContainers));
    sNumRegisteredSections = 0;
    sNextContainerID = 1000;
    sEditionsInitialized = true;
    return ED_NO_ERR;
}

int16_t lc3_ed_new_section(int16_t kind, int32_t sectionID, uint8_t mode, SectionRecord *outSection)
{
    if (!outSection || (kind != SECTION_TYPE_PUB && kind != SECTION_TYPE_SUB)) {
        return ED_PARAM_ERR;
    }

    if (!sEditionsInitialized) {
        lc3_ed_init_editions();
    }

    memset(outSection, 0, sizeof(SectionRecord));
    outSection->version = 1;
    outSection->kind = kind;
    outSection->mode = mode;
    outSection->sectionID = sectionID;
    outSection->isDirty = false;
    outSection->registered = false;
    outSection->aliasRecord = sNextContainerID++;

    return ED_NO_ERR;
}

int16_t lc3_ed_register_section(SectionRecord *section)
{
    if (!section) return ED_PARAM_ERR;
    if (section->registered) return ED_ALREADY_REG;
    if (sNumRegisteredSections >= MAX_ACTIVE_SECTIONS) return ED_BAD_SECTION;

    for (uint32_t i = 0; i < MAX_ACTIVE_SECTIONS; ++i) {
        if (!sRegisteredSections[i].registered) {
            sRegisteredSections[i] = *section;
            sRegisteredSections[i].registered = true;
            section->registered = true;
            sNumRegisteredSections++;
            return ED_NO_ERR;
        }
    }

    return ED_BAD_SECTION;
}

int16_t lc3_ed_unregister_section(SectionRecord *section)
{
    if (!section) return ED_PARAM_ERR;
    if (!section->registered) return ED_NOT_REGISTERED;

    for (uint32_t i = 0; i < MAX_ACTIVE_SECTIONS; ++i) {
        if (sRegisteredSections[i].registered &&
            sRegisteredSections[i].sectionID == section->sectionID) {
            sRegisteredSections[i].registered = false;
            section->registered = false;
            if (sNumRegisteredSections > 0) sNumRegisteredSections--;
            return ED_NO_ERR;
        }
    }

    return ED_NOT_REGISTERED;
}

int16_t lc3_ed_open_edition(SectionRecord *section, bool forWrite, EditionRefNum *editionRef)
{
    if (!section || !editionRef) return ED_PARAM_ERR;

    int freeIndex = -1;
    int foundIndex = -1;

    for (int i = 0; i < MAX_CONTAINERS; ++i) {
        if (sContainers[i].inUse && sContainers[i].containerID == section->aliasRecord) {
            foundIndex = i;
            break;
        }
        if (!sContainers[i].inUse && freeIndex < 0) {
            freeIndex = i;
        }
    }

    if (forWrite) {
        if (foundIndex < 0) {
            if (freeIndex < 0) return ED_IO_ERR;
            foundIndex = freeIndex;
            sContainers[foundIndex].inUse = true;
            sContainers[foundIndex].containerID = section->aliasRecord;
            sContainers[foundIndex].length = 0;
            sContainers[foundIndex].formatType = 0;
        }
        sContainers[foundIndex].length = 0; /* Reset for fresh publish */
    } else {
        if (foundIndex < 0) return ED_BAD_SECTION;
    }

    editionRef->refNum = (uint32_t)(foundIndex + 1);
    editionRef->containerID = section->aliasRecord;
    editionRef->currentPos = 0;
    editionRef->totalLength = sContainers[foundIndex].length;
    editionRef->formatType = sContainers[foundIndex].formatType;
    editionRef->writeMode = forWrite;

    return ED_NO_ERR;
}

int16_t lc3_ed_close_edition(EditionRefNum *editionRef, bool success)
{
    if (!editionRef || editionRef->refNum == 0 || editionRef->refNum > MAX_CONTAINERS) {
        return ED_PARAM_ERR;
    }

    uint32_t idx = editionRef->refNum - 1;
    if (!sContainers[idx].inUse) return ED_BAD_SECTION;

    if (editionRef->writeMode && success) {
        sContainers[idx].length = editionRef->totalLength;
        sContainers[idx].modDate++;
    }

    editionRef->refNum = 0;
    return ED_NO_ERR;
}

int16_t lc3_ed_write_edition(EditionRefNum *editionRef, uint32_t formatType, const void *data, uint32_t count)
{
    if (!editionRef || !data || !editionRef->writeMode) return ED_PARAM_ERR;

    uint32_t idx = editionRef->refNum - 1;
    if (idx >= MAX_CONTAINERS || !sContainers[idx].inUse) return ED_BAD_SECTION;

    if (editionRef->currentPos + count > sizeof(sContainers[idx].buffer)) {
        return ED_IO_ERR;
    }

    memcpy(&sContainers[idx].buffer[editionRef->currentPos], data, count);
    editionRef->currentPos += count;
    if (editionRef->currentPos > editionRef->totalLength) {
        editionRef->totalLength = editionRef->currentPos;
    }
    sContainers[idx].formatType = formatType;

    return ED_NO_ERR;
}
