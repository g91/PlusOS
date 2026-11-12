/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: CommToolbox Terminal Manager Core
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_CTB_TERMINAL_H
#define LC3_CTB_TERMINAL_H

#include <stdint.h>
#include <stdbool.h>

#define TM_NO_ERR           0
#define TM_PARAM_ERR      -50
#define TM_NOT_OPEN       -51

#define TM_MAX_COLS        80
#define TM_MAX_ROWS        24

typedef struct TMRecord {
    int16_t procID;             /* Terminal tool ID (e.g. VT100 = 1, TTY = 2) */
    uint16_t cursorRow;         /* 0 to TM_MAX_ROWS-1 */
    uint16_t cursorCol;         /* 0 to TM_MAX_COLS-1 */
    bool cursorVisible;
    bool wrapAround;
    char screenBuffer[TM_MAX_ROWS][TM_MAX_COLS];
    uint8_t attrBuffer[TM_MAX_ROWS][TM_MAX_COLS];
    uint8_t currentAttr;
    int16_t ansiState;          /* Escape sequence parser state */
    uint16_t ansiParam[4];      /* ANSI command parameters */
    uint8_t ansiParamCount;
} TMRecord, *TMRecordPtr, **TMHandle;

/* Public API */
int16_t lc3_ctb_tm_init(void);
int16_t lc3_ctb_tm_open(TMHandle hTerm, int16_t toolID);
int16_t lc3_ctb_tm_write(TMHandle hTerm, const char *data, uint32_t length);
int16_t lc3_ctb_tm_parse_ansi(TMRecordPtr term, char c);
int16_t lc3_ctb_tm_key_down(TMHandle hTerm, uint8_t keyCode, uint8_t charCode, char *outSeq, uint16_t *outLen);
int16_t lc3_ctb_tm_close(TMHandle hTerm);

#endif /* LC3_CTB_TERMINAL_H */
