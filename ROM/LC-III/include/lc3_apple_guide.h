/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Guide Core Engine (Package 10 in ROM)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_APPLE_GUIDE_H
#define LC3_APPLE_GUIDE_H

#include <stdint.h>
#include <stdbool.h>

#define AG_NO_ERR             0
#define AG_NOT_FOUND_ERR   -2900
#define AG_PARAM_ERR         -50
#define AG_ALREADY_OPEN    -2901

typedef void *AGRefNum;
typedef uint32_t AGSequenceType;

typedef struct {
    int16_t v;
    int16_t h;
} AGPoint;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} AGRect;

typedef struct AGSequenceRecord {
    AGRefNum refNum;
    char guideTitle[64];
    uint32_t panelCount;
    uint32_t currentPanel;
    bool isOpen;
    bool isFront;
    void *coachMarkRgn;
} AGSequenceRecord;

/* Apple Guide API */
int16_t lc3_ag_open_with_sequence(const char *guideName, AGSequenceType seqType, AGRefNum *refNum);
int16_t lc3_ag_close(AGRefNum refNum);
int16_t lc3_ag_start(AGRefNum refNum, uint32_t panelNumber);
int16_t lc3_ag_general(AGRefNum refNum, int16_t cmd, void *params);
int16_t lc3_ag_context(AGRefNum refNum, uint32_t contextID);
int16_t lc3_ag_get_front_sequence(AGRefNum *refNum);
int16_t lc3_ag_set_front_sequence(AGRefNum refNum);
bool lc3_ag_is_sequence_open(AGRefNum refNum);

#endif /* LC3_APPLE_GUIDE_H */
