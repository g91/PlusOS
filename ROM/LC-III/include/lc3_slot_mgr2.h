/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Slot Manager 2.0 Extended PDS Slot Declarations & sResources
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SLOT_MGR2_H
#define LC3_SLOT_MGR2_H

#include <stdint.h>
#include <stdbool.h>

#define SLOT_NO_ERR         0
#define SLOT_NOT_FOUND   -300
#define SLOT_PARAM_ERR    -50

typedef struct SpBlock {
    void *spsPointer;
    void *spParamData;
    int32_t spSize;
    int32_t spOffsetData;
    void *spIOFileName;
    void *spsExecPBlk;
    int16_t spSlot;
    int16_t spId;
    int16_t spExtDev;
    int8_t  spHwDev;
    int8_t  spByteLanes;
    int8_t  spFlags;
    int8_t  spKey;
    int16_t spStatus;
    int32_t spResult;
} SpBlock, *SpBlockPtr;

/* Slot Manager 2.0 API */
int16_t lc3_s_rsrc_info(SpBlockPtr sp);
int16_t lc3_s_search(SpBlockPtr sp);
int16_t lc3_s_find_struct(SpBlockPtr sp);
int16_t lc3_s_read_byte(SpBlockPtr sp, uint8_t *val);
int16_t lc3_s_read_word(SpBlockPtr sp, uint16_t *val);
int16_t lc3_s_read_long(SpBlockPtr sp, uint32_t *val);
int16_t lc3_s_get_block(SpBlockPtr sp, void *dest, int32_t size);

#endif /* LC3_SLOT_MGR2_H */
