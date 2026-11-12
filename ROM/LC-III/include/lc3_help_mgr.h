/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Help Manager (Balloon Help Engine)
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_HELP_MGR_H
#define LC3_HELP_MGR_H

#include <stdint.h>
#include <stdbool.h>

#define HM_BALLOON_OFF       0
#define HM_BALLOON_ON        1

#define HM_DEFAULT_PROC      0
#define HM_SAVE_BITS_PROC    1

typedef struct {
    int16_t v;
    int16_t h;
} HMPoint;

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} HMRect;

typedef struct {
    int16_t hmmType;
    union {
        char hmmString[256];
        int16_t hmmPict;
        struct {
            int16_t resID;
            int16_t index;
        } hmmStrRes;
    } u;
} HMMessageRecord;

/* Help Manager API */
bool lc3_hm_get_balloons(void);
void lc3_hm_set_balloons(bool enabled);
bool lc3_hm_is_balloon(void);
int16_t lc3_hm_remove_balloon(void);
void *lc3_hm_get_help_menu_handle(void);
int16_t lc3_hm_show_balloon(const HMMessageRecord *aHelpMsg, HMPoint tip, const HMRect *alternateRect, void *tipProc, int16_t theProc, int16_t balloonVariant, int16_t method);
int16_t lc3_hm_extract_help_msg(int16_t whichType, int16_t whichResID, int16_t whichMsg, int16_t whichState, HMMessageRecord *aHelpMsg);

/* Help Manager Trap Dispatch ($A830, $A834, $A835) */
int32_t lc3_pack14_dispatch(int16_t selector, void *params);

#endif /* LC3_HELP_MGR_H */
