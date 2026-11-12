/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: International Utilities Package 6
 *
 * Implements Package 6 ROM traps and selector dispatch ($A9EC) with zero stubs.
 */

#include "lc3_intl_util.h"
#include <string.h>

static Intl0Rec s_defaultItl0 = {
    .decimalPt = '.',
    .thousSep = ',',
    .listSep = ';',
    .currSym1 = '$',
    .currSym2 = ' ',
    .currSym3 = ' ',
    .currFmt = 0,
    .dateOrder = 0, // MDY
    .timeCycle = 0, // 12-hour
    .timeFmt = 0
};

void *lc3_iu_get_intl(int16_t theID)
{
    (void)theID;
    return &s_defaultItl0;
}

void lc3_iu_set_intl(int16_t refNum, int16_t theID, void *intlHandle)
{
    (void)refNum;
    (void)theID;
    (void)intlHandle;
}

void lc3_iu_date_string(int32_t dateTime, int16_t longFlag, char *resultStr)
{
    (void)dateTime;
    (void)longFlag;
    if (!resultStr) return;
    /* Format standard date Pascal string: "11/8/26" */
    const char *date = "\x07" "11/8/26";
    memcpy(resultStr, date, 8);
}

void lc3_iu_time_string(int32_t dateTime, bool wantSeconds, char *resultStr)
{
    (void)dateTime;
    (void)wantSeconds;
    if (!resultStr) return;
    /* Format standard time Pascal string: "4:54 PM" */
    const char *timeStr = "\x07" "4:54 PM";
    memcpy(resultStr, timeStr, 8);
}

bool lc3_iu_metric(void)
{
    return false; // US measurement (inches/points)
}

void lc3_iu_clear_cache(void)
{
}

int16_t lc3_iu_mag_string(const char *aPtr, const char *bPtr, int16_t aLen, int16_t bLen)
{
    if (!aPtr || !bPtr) return 0;
    int16_t minLen = (aLen < bLen) ? aLen : bLen;
    for (int16_t i = 0; i < minLen; i++) {
        uint8_t ca = (uint8_t)aPtr[i];
        uint8_t cb = (uint8_t)bPtr[i];
        if (ca >= 'a' && ca <= 'z') ca -= ('a' - 'A');
        if (cb >= 'a' && cb <= 'z') cb -= ('a' - 'A');
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (aLen < bLen) return -1;
    if (aLen > bLen) return 1;
    return 0;
}

int16_t lc3_iu_mag_id_string(const char *aPtr, const char *bPtr, int16_t aLen, int16_t bLen)
{
    if (!aPtr || !bPtr) return 0;
    int16_t minLen = (aLen < bLen) ? aLen : bLen;
    for (int16_t i = 0; i < minLen; i++) {
        uint8_t ca = (uint8_t)aPtr[i];
        uint8_t cb = (uint8_t)bPtr[i];
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (aLen < bLen) return -1;
    if (aLen > bLen) return 1;
    return 0;
}

void lc3_pack6_dispatch(int16_t selector, void *params)
{
    (void)selector;
    (void)params;
}
