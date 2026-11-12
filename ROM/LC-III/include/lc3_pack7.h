/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Binary-Decimal & Floating-Point Conversion Package 7
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_PACK7_H
#define LC3_PACK7_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t bytes[10]; // 80-bit IEEE extended float
} Extended80;

typedef struct {
    int16_t expDigits;
    int16_t minDigits;
    int16_t decDigits;
    int16_t signs;
    char fillChar;
} DecFormat;

/* Package 7 API */
void lc3_num_to_string(int32_t theNum, char *theString);
void lc3_string_to_num(const char *theString, int32_t *theNum);
void lc3_extended_to_string(const Extended80 *x, const DecFormat *d, char *theString);
void lc3_string_to_extended(const char *theString, const DecFormat *d, Extended80 *x);
void lc3_format_to_str(const DecFormat *d, const char *formatString, char *theString);
void lc3_str_to_format(const char *formatString, DecFormat *d);

/* Package 7 Dispatcher ($A9EE) */
void lc3_pack7_dispatch(int16_t selector, void *params);

#endif /* LC3_PACK7_H */
