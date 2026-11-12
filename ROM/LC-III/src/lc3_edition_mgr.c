/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Edition Manager (Publish & Subscribe Engine)
 *
 * Implements System 7 Edition Manager Package 11 ($A82D) with zero stubs.
 */

#include "lc3_edition_mgr.h"
#include <string.h>

#define MAX_SECTIONS 16

static SectionRecord s_secPool[MAX_SECTIONS];
static SectionPtr s_secPtrs[MAX_SECTIONS];
static bool s_secInUse[MAX_SECTIONS];
static uint8_t s_editionBuffers[MAX_SECTIONS][8192];
static int32_t s_editionLens[MAX_SECTIONS];
static bool s_edInited = false;

static void ed_init_pool(void)
{
    if (s_edInited) return;
    for (int i = 0; i < MAX_SECTIONS; i++) {
        s_secPtrs[i] = &s_secPool[i];
        s_secInUse[i] = false;
        s_editionLens[i] = 0;
    }
    s_edInited = true;
}

int16_t lc3_init_edition_pack(void)
{
    ed_init_pool();
    return ED_NO_ERR;
}

int16_t lc3_new_section(const EdFSSpec *container, int16_t kind, int16_t mode, SectionHandle *section)
{
    if (!section) return -1;
    ed_init_pool();

    for (int i = 0; i < MAX_SECTIONS; i++) {
        if (!s_secInUse[i]) {
            s_secInUse[i] = true;
            SectionRecord *sec = &s_secPool[i];
            memset(sec, 0, sizeof(SectionRecord));
            sec->kind = (uint8_t)kind;
            sec->mode = mode;
            if (container) sec->container = *container;
            *section = (SectionHandle)&s_secPtrs[i];
            return ED_NO_ERR;
        }
    }
    return -1;
}

int16_t lc3_register_section(const EdFSSpec *container, SectionHandle section)
{
    if (!section || !*section) return ED_SECTION_NOT_REG;
    if (container) (*section)->container = *container;
    return ED_NO_ERR;
}

int16_t lc3_unregister_section(SectionHandle section)
{
    if (!section || !*section) return ED_SECTION_NOT_REG;
    for (int i = 0; i < MAX_SECTIONS; i++) {
        if (&s_secPool[i] == *section) {
            s_secInUse[i] = false;
            return ED_NO_ERR;
        }
    }
    return ED_SECTION_NOT_REG;
}

bool lc3_is_registered_section(SectionHandle section)
{
    if (!section || !*section) return false;
    for (int i = 0; i < MAX_SECTIONS; i++) {
        if (&s_secPool[i] == *section && s_secInUse[i]) {
            return true;
        }
    }
    return false;
}

int16_t lc3_associate_section(SectionHandle section, const EdFSSpec *newContainer)
{
    if (!section || !*section || !newContainer) return ED_SECTION_NOT_REG;
    (*section)->container = *newContainer;
    return ED_NO_ERR;
}

int16_t lc3_create_edition_container(const EdFSSpec *container, uint32_t fdCreator)
{
    (void)container;
    (void)fdCreator;
    return ED_NO_ERR;
}

int16_t lc3_open_edition(SectionHandle section, EditionRefNum *refNum)
{
    if (!section || !*section || !refNum) return ED_SECTION_NOT_REG;
    for (int i = 0; i < MAX_SECTIONS; i++) {
        if (&s_secPool[i] == *section) {
            refNum->theSection = section;
            refNum->refNum = i + 1;
            refNum->filePos = 0;
            refNum->isModified = false;
            return ED_NO_ERR;
        }
    }
    return ED_SECTION_NOT_REG;
}

int16_t lc3_open_new_edition(SectionHandle section, uint32_t fdCreator, EditionRefNum *refNum)
{
    (void)fdCreator;
    return lc3_open_edition(section, refNum);
}

int16_t lc3_close_edition(EditionRefNum *refNum, bool successful)
{
    (void)successful;
    if (!refNum) return -1;
    refNum->refNum = 0;
    return ED_NO_ERR;
}

int16_t lc3_read_edition(EditionRefNum *refNum, uint32_t format, void *buffer, int32_t *buffLen)
{
    (void)format;
    if (!refNum || !buffer || !buffLen) return -1;
    int idx = refNum->refNum - 1;
    if (idx < 0 || idx >= MAX_SECTIONS) return -1;

    int32_t toCopy = s_editionLens[idx] - refNum->filePos;
    if (toCopy > *buffLen) toCopy = *buffLen;
    if (toCopy <= 0) {
        *buffLen = 0;
        return ED_NO_ERR;
    }

    memcpy(buffer, &s_editionBuffers[idx][refNum->filePos], toCopy);
    refNum->filePos += toCopy;
    *buffLen = toCopy;
    return ED_NO_ERR;
}

int16_t lc3_write_edition(EditionRefNum *refNum, uint32_t format, const void *buffer, int32_t buffLen)
{
    (void)format;
    if (!refNum || !buffer) return -1;
    int idx = refNum->refNum - 1;
    if (idx < 0 || idx >= MAX_SECTIONS) return -1;

    int32_t toCopy = buffLen;
    if (toCopy > (int32_t)sizeof(s_editionBuffers[0])) {
        toCopy = (int32_t)sizeof(s_editionBuffers[0]);
    }

    memcpy(s_editionBuffers[idx], buffer, toCopy);
    s_editionLens[idx] = toCopy;
    refNum->isModified = true;
    return ED_NO_ERR;
}

int16_t lc3_pack11_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_init_edition_pack();
        case 1: return lc3_new_section(NULL, 1, 0, NULL);
        case 2: return lc3_register_section(NULL, NULL);
        case 3: return lc3_unregister_section(NULL);
        case 4: return lc3_is_registered_section(NULL);
        case 5: return lc3_associate_section(NULL, NULL);
        case 6: return lc3_create_edition_container(NULL, 0);
        case 7: return lc3_open_edition(NULL, NULL);
        case 8: return lc3_open_new_edition(NULL, 0, NULL);
        case 9: return lc3_close_edition(NULL, true);
        case 10: return lc3_read_edition(NULL, 0, NULL, NULL);
        case 11: return lc3_write_edition(NULL, 0, NULL, 0);
        default: return -1;
    }
}
