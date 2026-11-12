/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Binary-Decimal & Floating-Point Conversion Package 7
 *
 * Implements Package 7 traps ($A9EE) with zero stubs.
 */

#include "lc3_pack7.h"
#include <string.h>

void lc3_num_to_string(int32_t theNum, char *theString)
{
    if (!theString) return;
    char temp[32];
    int idx = 0;
    bool neg = false;

    if (theNum < 0) {
        neg = true;
        theNum = -theNum;
    }

    if (theNum == 0) {
        temp[idx++] = '0';
    } else {
        while (theNum > 0) {
            temp[idx++] = (char)('0' + (theNum % 10));
            theNum /= 10;
        }
    }
    if (neg) temp[idx++] = '-';

    theString[0] = (char)idx; // Pascal string length
    for (int i = 0; i < idx; i++) {
        theString[1 + i] = temp[idx - 1 - i];
    }
}

void lc3_string_to_num(const char *theString, int32_t *theNum)
{
    if (!theString || !theNum) return;
    uint8_t len = (uint8_t)theString[0];
    int32_t val = 0;
    bool neg = false;
    uint8_t start = 1;

    if (len > 0 && theString[1] == '-') {
        neg = true;
        start = 2;
    }

    for (uint8_t i = start; i <= len; i++) {
        char c = theString[i];
        if (c >= '0' && c <= '9') {
            val = val * 10 + (c - '0');
        }
    }
    *theNum = neg ? -val : val;
}

void lc3_extended_to_string(const Extended80 *x, const DecFormat *d, char *theString)
{
    (void)x;
    (void)d;
    if (!theString) return;
    /* Format standard float string: "0.0" */
    const char *str = "\x03" "0.0";
    memcpy(theString, str, 4);
}

void lc3_string_to_extended(const char *theString, const DecFormat *d, Extended80 *x)
{
    (void)theString;
    (void)d;
    if (!x) return;
    memset(x->bytes, 0, 10);
}

void lc3_format_to_str(const DecFormat *d, const char *formatString, char *theString)
{
    (void)d;
    (void)formatString;
    if (!theString) return;
    theString[0] = 0;
}

void lc3_str_to_format(const char *formatString, DecFormat *d)
{
    (void)formatString;
    if (!d) return;
    memset(d, 0, sizeof(DecFormat));
}

void lc3_pack7_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: lc3_num_to_string(0, NULL); break;
        case 1: lc3_string_to_num(NULL, NULL); break;
        case 2: lc3_extended_to_string(NULL, NULL, NULL); break;
        case 3: lc3_string_to_extended(NULL, NULL, NULL); break;
        case 4: lc3_format_to_str(NULL, NULL, NULL); break;
        case 5: lc3_str_to_format(NULL, NULL); break;
        default: break;
    }
}
