/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Disk Initialization Package 2
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DISK_INIT_H
#define LC3_DISK_INIT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t v;
    int16_t h;
} DIPoint;

/* Package 2 API */
int16_t lc3_di_bad_mount(DIPoint where, int32_t evtMessage);
int16_t lc3_di_format(int16_t drvNum);
int16_t lc3_di_verify(int16_t drvNum);
int16_t lc3_di_zero(int16_t drvNum, const uint8_t *volName);

/* Package 2 Dispatcher */
int16_t lc3_pack2_dispatch(int16_t selector, void *params);

#endif /* LC3_DISK_INIT_H */
