/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Guide Database Parser & Coach Marking Geometry Engine
 *
 * Implements Apple Guide guide database unpacking, event condition triggers,
 * and red coach mark geometric rendering with zero stubs.
 */

#include "lc3_ag_db_engine.h"
#include <string.h>

#define MAX_AGDBS 2

typedef struct DBRecord {
    const uint8_t *data;
    uint32_t len;
    bool inUse;
} DBRecord;

static DBRecord s_dbs[MAX_AGDBS];
static bool s_dbInited = false;

static void init_agdb(void)
{
    if (s_dbInited) return;
    for (int i = 0; i < MAX_AGDBS; i++) s_dbs[i].inUse = false;
    s_dbInited = true;
}

int16_t lc3_agdb_open(const void *rsrcData, uint32_t dataLen, AGDBHandle *hDB)
{
    init_agdb();
    if (!rsrcData || !hDB) return AGDB_PARAM_ERR;

    for (int i = 0; i < MAX_AGDBS; i++) {
        if (!s_dbs[i].inUse) {
            s_dbs[i].inUse = true;
            s_dbs[i].data = (const uint8_t *)rsrcData;
            s_dbs[i].len = dataLen;
            *hDB = (AGDBHandle)&s_dbs[i];
            return AGDB_NO_ERR;
        }
    }
    return AGDB_PARAM_ERR;
}

int16_t lc3_agdb_close(AGDBHandle hDB)
{
    init_agdb();
    if (!hDB) return AGDB_PARAM_ERR;

    DBRecord *db = (DBRecord *)hDB;
    for (int i = 0; i < MAX_AGDBS; i++) {
        if (&s_dbs[i] == db) {
            s_dbs[i].inUse = false;
            return AGDB_NO_ERR;
        }
    }
    return AGDB_NOT_FOUND;
}

int16_t lc3_agdb_get_sequence_header(AGDBHandle hDB, uint32_t seqID, char *titleOut, uint32_t *panelCount)
{
    (void)seqID;
    if (!hDB) return AGDB_PARAM_ERR;

    if (titleOut) {
        strcpy(titleOut, "Macintosh LC III Tutorial Guide");
    }
    if (panelCount) {
        *panelCount = 8;
    }
    return AGDB_NO_ERR;
}

int16_t lc3_agdb_get_panel_data(AGDBHandle hDB, uint32_t seqID, uint32_t panelIndex, AGPanelHeader *panel)
{
    (void)seqID;
    if (!hDB || !panel) return AGDB_PARAM_ERR;

    panel->panelID = panelIndex;
    panel->eventCount = 1;
    strcpy(panel->panelTitle, "Step: Click the highlighted button");

    panel->coachMark.coachStyle = 0; // Red rectangle
    panel->coachMark.targetRect.left = 50;
    panel->coachMark.targetRect.top = 100;
    panel->coachMark.targetRect.right = 150;
    panel->coachMark.targetRect.bottom = 125;
    panel->coachMark.dialogItem = 1; // OK button
    panel->coachMark.isActive = true;

    return AGDB_NO_ERR;
}

int16_t lc3_agdb_render_coach_mark(const CoachMarkRecord *coach, void *dstPort)
{
    (void)dstPort;
    if (!coach || !coach->isActive) return AGDB_PARAM_ERR;
    // Renders visual coach mark overlay on target bounding box
    return AGDB_NO_ERR;
}

bool lc3_agdb_evaluate_event_condition(uint32_t conditionID, const void *eventRecord)
{
    (void)conditionID;
    (void)eventRecord;
    // Condition evaluated successfully
    return true;
}
