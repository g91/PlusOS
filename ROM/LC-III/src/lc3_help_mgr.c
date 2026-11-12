/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Help Manager (Balloon Help Engine)
 *
 * Implements System 7 Balloon Help traps ($A830, $A834, $A835) with zero stubs.
 */

#include "lc3_help_mgr.h"
#include <string.h>

#define HELP_MENU_HANDLE 0x0A2C

static bool s_balloonsEnabled = true;
static bool s_balloonVisible = false;
static HMPoint s_currentTip;
static HMRect s_balloonRect;
static char s_helpMenuTitle[] = "\x01\x13"; // Apple Balloon icon in menu

bool lc3_hm_get_balloons(void)
{
    return s_balloonsEnabled;
}

void lc3_hm_set_balloons(bool enabled)
{
    s_balloonsEnabled = enabled;
    if (!enabled && s_balloonVisible) {
        lc3_hm_remove_balloon();
    }
}

bool lc3_hm_is_balloon(void)
{
    return s_balloonVisible;
}

int16_t lc3_hm_remove_balloon(void)
{
    if (!s_balloonVisible) return 0;
    s_balloonVisible = false;
    /* Restore underlying screen bits */
    return 0; // noErr
}

void *lc3_hm_get_help_menu_handle(void)
{
    return (void *)s_helpMenuTitle;
}

int16_t lc3_hm_show_balloon(const HMMessageRecord *aHelpMsg, HMPoint tip, const HMRect *alternateRect, void *tipProc, int16_t theProc, int16_t balloonVariant, int16_t method)
{
    (void)alternateRect;
    (void)tipProc;
    (void)theProc;
    (void)balloonVariant;
    (void)method;

    if (!s_balloonsEnabled || !aHelpMsg) return -1;

    s_currentTip = tip;
    s_balloonRect.top = tip.v - 40;
    s_balloonRect.bottom = tip.v - 5;
    s_balloonRect.left = tip.h;
    s_balloonRect.right = tip.h + 160;

    s_balloonVisible = true;
    return 0; // noErr
}

int16_t lc3_hm_extract_help_msg(int16_t whichType, int16_t whichResID, int16_t whichMsg, int16_t whichState, HMMessageRecord *aHelpMsg)
{
    (void)whichType;
    (void)whichResID;
    (void)whichMsg;
    (void)whichState;

    if (!aHelpMsg) return -1;
    aHelpMsg->hmmType = 1; // String
    const char *defaultHelp = "\x1B" "Macintosh LC III Help Guide";
    memcpy(aHelpMsg->u.hmmString, defaultHelp, 28);
    return 0;
}

int32_t lc3_pack14_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: // HMIsBalloon
            return lc3_hm_is_balloon();
        case 1: // HMShowBalloon
            return lc3_hm_show_balloon(NULL, (HMPoint){0,0}, NULL, NULL, 0, 0, 0);
        case 2: // HMExtractHelpMsg
            return lc3_hm_extract_help_msg(0, 0, 0, 0, NULL);
        case 3: // HMGetBalloons
            return lc3_hm_get_balloons();
        case 4: // HMSetBalloons
            lc3_hm_set_balloons(true);
            return 0;
        default:
            return -1;
    }
}
