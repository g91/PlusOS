/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: International Utilities Package 6
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_INTL_UTIL_H
#define LC3_INTL_UTIL_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t decimalPt;
    int16_t thousSep;
    int16_t listSep;
    int16_t currSym1;
    int16_t currSym2;
    int16_t currSym3;
    uint8_t currFmt;
    uint8_t dateOrder;
    int16_t timeCycle;
    int16_t timeFmt;
} Intl0Rec;

/* Package 6 API */
void *lc3_iu_get_intl(int16_t theID);
void lc3_iu_set_intl(int16_t refNum, int16_t theID, void *intlHandle);
void lc3_iu_date_string(int32_t dateTime, int16_t longFlag, char *resultStr);
void lc3_iu_time_string(int32_t dateTime, bool wantSeconds, char *resultStr);
bool lc3_iu_metric(void);
void lc3_iu_clear_cache(void);
int16_t lc3_iu_mag_string(const char *aPtr, const char *bPtr, int16_t aLen, int16_t bLen);
int16_t lc3_iu_mag_id_string(const char *aPtr, const char *bPtr, int16_t aLen, int16_t bLen);

/* Package 6 Dispatcher */
void lc3_pack6_dispatch(int16_t selector, void *params);

#endif /* LC3_INTL_UTIL_H */
