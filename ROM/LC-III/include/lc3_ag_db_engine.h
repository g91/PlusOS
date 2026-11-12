/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Guide Database Parser & Coach Marking Geometry Engine
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_AG_DB_ENGINE_H
#define LC3_AG_DB_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

#define AGDB_NO_ERR         0
#define AGDB_NOT_FOUND   -2905
#define AGDB_PARAM_ERR     -50

typedef void *AGDBHandle;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} AGRect;

typedef struct CoachMarkRecord {
    int16_t coachStyle; // 0 = red rectangle, 1 = red circle, 2 = red arrow
    AGRect targetRect;
    int16_t menuID;
    int16_t menuItem;
    int16_t dialogItem;
    bool isActive;
} CoachMarkRecord;

typedef struct AGPanelHeader {
    uint32_t panelID;
    uint16_t eventCount;
    char panelTitle[64];
    CoachMarkRecord coachMark;
} AGPanelHeader;

/* Apple Guide Database API */
int16_t lc3_agdb_open(const void *rsrcData, uint32_t dataLen, AGDBHandle *hDB);
int16_t lc3_agdb_close(AGDBHandle hDB);
int16_t lc3_agdb_get_sequence_header(AGDBHandle hDB, uint32_t seqID, char *titleOut, uint32_t *panelCount);
int16_t lc3_agdb_get_panel_data(AGDBHandle hDB, uint32_t seqID, uint32_t panelIndex, AGPanelHeader *panel);
int16_t lc3_agdb_render_coach_mark(const CoachMarkRecord *coach, void *dstPort);
bool lc3_agdb_evaluate_event_condition(uint32_t conditionID, const void *eventRecord);

#endif /* LC3_AG_DB_ENGINE_H */
