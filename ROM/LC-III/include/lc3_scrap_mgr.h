/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Scrap Manager (Clipboard) Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_SCRAP_MGR_H
#define LC3_SCRAP_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define NO_ERR         0
#define NO_TYPE_ERR   -102
#define MEM_FULL_ERR  -108
#define NIL_HANDLE_ERR -109

typedef uint32_t ResType;

typedef struct {
    int32_t scrapSize;
    void *scrapHandle;
    int16_t scrapCount;
    int16_t scrapState;
    char *scrapName;
} ScrapInfo, *PScrapInfo;

/* Scrap Manager API */
int32_t lc3_info_scrap(ScrapInfo *info);
int32_t lc3_zero_scrap(void);
int32_t lc3_put_scrap(int32_t length, ResType theType, const void *source);
int32_t lc3_get_scrap(void *hDest, ResType theType, int32_t *offset);
int32_t lc3_load_scrap(void);
int32_t lc3_unload_scrap(void);

#endif /* LC3_SCRAP_MGR_H */
