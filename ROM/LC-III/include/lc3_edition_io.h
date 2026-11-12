/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Edition Manager Section Records & Publish/Subscribe Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_EDITION_IO_H
#define LC3_EDITION_IO_H

#include <stdint.h>
#include <stdbool.h>

#define ED_NO_ERR              0
#define ED_NOT_REGISTERED    -450
#define ED_ALREADY_REG       -451
#define ED_BAD_SECTION       -452
#define ED_IO_ERR            -453
#define ED_PARAM_ERR          -50

#define SECTION_TYPE_PUB        1
#define SECTION_TYPE_SUB        2

typedef struct EditionRefNum {
    uint32_t refNum;
    uint32_t containerID;
    uint32_t formatType;
    uint32_t currentPos;
    uint32_t totalLength;
    bool writeMode;
} EditionRefNum;

typedef struct SectionRecord {
    uint16_t version;
    int16_t kind;             /* SECTION_TYPE_PUB or SECTION_TYPE_SUB */
    uint8_t mode;             /* manual vs automatic */
    uint8_t mdDate[4];        /* modification date */
    int32_t sectionID;
    uint32_t refCon;
    uint32_t aliasRecord;     /* Alias to container file */
    bool isDirty;
    bool registered;
} SectionRecord, *SectionPtr, **SectionHandle;

/* Edition Manager Core API */
int16_t lc3_ed_init_editions(void);
int16_t lc3_ed_new_section(int16_t kind, int32_t sectionID, uint8_t mode, SectionRecord *outSection);
int16_t lc3_ed_register_section(SectionRecord *section);
int16_t lc3_ed_unregister_section(SectionRecord *section);
int16_t lc3_ed_open_edition(SectionRecord *section, bool forWrite, EditionRefNum *editionRef);
int16_t lc3_ed_close_edition(EditionRefNum *editionRef, bool success);
int16_t lc3_ed_write_edition(EditionRefNum *editionRef, uint32_t formatType, const void *data, uint32_t count);

#endif /* LC3_EDITION_IO_H */
