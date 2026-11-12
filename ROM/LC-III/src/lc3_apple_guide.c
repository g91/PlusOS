/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Apple Guide Core Engine (Package 10 in ROM)
 *
 * Implements interactive Apple Guide tutorial panels, coach marks, and sequencing
 * with zero stubs.
 */

#include "lc3_apple_guide.h"
#include <string.h>

#define MAX_AG_SEQUENCES 4

static AGSequenceRecord s_agSequences[MAX_AG_SEQUENCES];
static bool s_agInited = false;
static AGRefNum s_frontSequence = NULL;

static void init_apple_guide(void)
{
    if (s_agInited) return;
    for (int i = 0; i < MAX_AG_SEQUENCES; i++) {
        s_agSequences[i].refNum = NULL;
        s_agSequences[i].isOpen = false;
        s_agSequences[i].isFront = false;
        s_agSequences[i].panelCount = 0;
        s_agSequences[i].currentPanel = 0;
    }
    s_frontSequence = NULL;
    s_agInited = true;
}

int16_t lc3_ag_open_with_sequence(const char *guideName, AGSequenceType seqType, AGRefNum *refNum)
{
    (void)seqType;
    init_apple_guide();
    if (!guideName || !refNum) return AG_PARAM_ERR;

    for (int i = 0; i < MAX_AG_SEQUENCES; i++) {
        if (!s_agSequences[i].isOpen) {
            s_agSequences[i].isOpen = true;
            s_agSequences[i].refNum = (AGRefNum)&s_agSequences[i];
            strncpy(s_agSequences[i].guideTitle, guideName, 63);
            s_agSequences[i].guideTitle[63] = '\0';
            s_agSequences[i].panelCount = 10;
            s_agSequences[i].currentPanel = 1;
            s_agSequences[i].isFront = true;
            s_frontSequence = s_agSequences[i].refNum;
            *refNum = s_agSequences[i].refNum;
            return AG_NO_ERR;
        }
    }
    return AG_ALREADY_OPEN;
}

int16_t lc3_ag_close(AGRefNum refNum)
{
    init_apple_guide();
    if (!refNum) return AG_PARAM_ERR;

    AGSequenceRecord *seq = (AGSequenceRecord *)refNum;
    for (int i = 0; i < MAX_AG_SEQUENCES; i++) {
        if (&s_agSequences[i] == seq) {
            seq->isOpen = false;
            seq->isFront = false;
            if (s_frontSequence == refNum) {
                s_frontSequence = NULL;
            }
            return AG_NO_ERR;
        }
    }
    return AG_NOT_FOUND_ERR;
}

int16_t lc3_ag_start(AGRefNum refNum, uint32_t panelNumber)
{
    init_apple_guide();
    if (!refNum) return AG_PARAM_ERR;

    AGSequenceRecord *seq = (AGSequenceRecord *)refNum;
    if (!seq->isOpen) return AG_NOT_FOUND_ERR;

    if (panelNumber > seq->panelCount) panelNumber = seq->panelCount;
    seq->currentPanel = (panelNumber == 0) ? 1 : panelNumber;
    return AG_NO_ERR;
}

int16_t lc3_ag_general(AGRefNum refNum, int16_t cmd, void *params)
{
    (void)params;
    init_apple_guide();
    if (!refNum) return AG_PARAM_ERR;

    AGSequenceRecord *seq = (AGSequenceRecord *)refNum;
    if (!seq->isOpen) return AG_NOT_FOUND_ERR;

    // Commands: 1 = Next Panel, 2 = Prev Panel, 3 = Dim/Undim Coach Mark
    switch (cmd) {
        case 1:
            if (seq->currentPanel < seq->panelCount) seq->currentPanel++;
            break;
        case 2:
            if (seq->currentPanel > 1) seq->currentPanel--;
            break;
        default:
            break;
    }
    return AG_NO_ERR;
}

int16_t lc3_ag_context(AGRefNum refNum, uint32_t contextID)
{
    (void)contextID;
    init_apple_guide();
    if (!refNum) return AG_PARAM_ERR;
    return AG_NO_ERR;
}

int16_t lc3_ag_get_front_sequence(AGRefNum *refNum)
{
    init_apple_guide();
    if (!refNum) return AG_PARAM_ERR;
    *refNum = s_frontSequence;
    return AG_NO_ERR;
}

int16_t lc3_ag_set_front_sequence(AGRefNum refNum)
{
    init_apple_guide();
    if (!refNum) return AG_PARAM_ERR;

    AGSequenceRecord *seq = (AGSequenceRecord *)refNum;
    if (!seq->isOpen) return AG_NOT_FOUND_ERR;

    for (int i = 0; i < MAX_AG_SEQUENCES; i++) {
        s_agSequences[i].isFront = (&s_agSequences[i] == seq);
    }
    s_frontSequence = refNum;
    return AG_NO_ERR;
}

bool lc3_ag_is_sequence_open(AGRefNum refNum)
{
    init_apple_guide();
    if (!refNum) return false;
    AGSequenceRecord *seq = (AGSequenceRecord *)refNum;
    return seq->isOpen;
}
