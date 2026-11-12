/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Edition Manager (Publish & Subscribe Engine)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_EDITION_MGR_H
#define LC3_EDITION_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define SECTION_PUBLISHER   1
#define SECTION_SUBSCRIBER  2

#define ED_NO_ERR           0
#define ED_SECTION_NOT_REG -450
#define ED_DUP_SECTION     -451
#define ED_NOT_EDITION_FILE -452

typedef struct {
    int16_t vRefNum;
    int32_t parID;
    uint8_t name[64];
} EdFSSpec;

typedef struct {
    uint8_t version;
    uint8_t kind; // SECTION_PUBLISHER or SECTION_SUBSCRIBER
    int16_t mode;
    int32_t mdDate;
    int32_t actionSubKey;
    int32_t refCon;
    EdFSSpec container;
} SectionRecord, *SectionPtr, **SectionHandle;

typedef struct {
    SectionHandle theSection;
    int32_t refNum;
    int32_t filePos;
    bool isModified;
} EditionRefNum;

/* Edition Manager API */
int16_t lc3_init_edition_pack(void);
int16_t lc3_new_section(const EdFSSpec *container, int16_t kind, int16_t mode, SectionHandle *section);
int16_t lc3_register_section(const EdFSSpec *container, SectionHandle section);
int16_t lc3_unregister_section(SectionHandle section);
bool lc3_is_registered_section(SectionHandle section);
int16_t lc3_associate_section(SectionHandle section, const EdFSSpec *newContainer);
int16_t lc3_create_edition_container(const EdFSSpec *container, uint32_t fdCreator);
int16_t lc3_open_edition(SectionHandle section, EditionRefNum *refNum);
int16_t lc3_open_new_edition(SectionHandle section, uint32_t fdCreator, EditionRefNum *refNum);
int16_t lc3_close_edition(EditionRefNum *refNum, bool successful);
int16_t lc3_read_edition(EditionRefNum *refNum, uint32_t format, void *buffer, int32_t *buffLen);
int16_t lc3_write_edition(EditionRefNum *refNum, uint32_t format, const void *buffer, int32_t buffLen);

/* Package 11 Dispatcher ($A82D) */
int16_t lc3_pack11_dispatch(int16_t selector, void *params);

#endif /* LC3_EDITION_MGR_H */
