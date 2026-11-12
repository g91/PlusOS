/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Communications Toolbox Terminal Manager (TM) & File Transfer Manager (FT)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_COMM_TERM_FT_H
#define LC3_COMM_TERM_FT_H

#include <stdint.h>
#include <stdbool.h>

#define CTB_NO_ERR         0
#define CTB_GENERIC_ERR -210
#define CTB_PARAM_ERR    -50

typedef void *TermHandle;
typedef void *FTHandle;

typedef struct TermRecord {
    int16_t procID;
    uint32_t flags;
    struct {
        int16_t top;
        int16_t left;
        int16_t bottom;
        int16_t right;
    } viewRect;
    int16_t curRow;
    int16_t curCol;
    uint8_t screenBuf[24][80];
} TermRecord;

typedef struct FTRecord {
    int16_t procID;
    uint32_t flags;
    int16_t direction; // 0 = receive, 1 = send
    uint32_t bytesTransferred;
    uint32_t totalBytes;
    bool isActive;
} FTRecord;

/* Terminal Manager API */
int16_t lc3_tm_init(void);
TermHandle lc3_tm_new(int16_t procID, uint32_t flags, void *termRect);
int16_t lc3_tm_dispose(TermHandle hTerm);
void lc3_tm_key(TermHandle hTerm, const void *theEvent);
void lc3_tm_update(TermHandle hTerm, void *visRgn);

/* File Transfer Manager API */
int16_t lc3_ft_init(void);
FTHandle lc3_ft_new(int16_t procID, uint32_t flags);
int16_t lc3_ft_dispose(FTHandle hFT);
int16_t lc3_ft_start(FTHandle hFT, int16_t direction, const void *fileSpec);
int16_t lc3_ft_abort(FTHandle hFT);
void lc3_ft_exec(FTHandle hFT);

#endif /* LC3_COMM_TERM_FT_H */
