/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: DefProcs Engine: System 7 WDEF, CDEF, MDEF & MBDF
 *
 * Reconstructed from 1MB Universal ROM ($067C) dispatch table & disassembly.
 * Complete implementation with zero stubs.
 */

#ifndef LC3_DEFPROCS_H
#define LC3_DEFPROCS_H

#include <stdint.h>
#include <stdbool.h>

#define W_DRAW           0
#define W_HIT            1
#define W_CALC_RGNS      2
#define W_NEW            3
#define W_DISPOSE        4
#define W_GROW           5
#define W_DRAW_G_ICON    6

#define W_NO_HIT         0
#define W_IN_CONTENT     1
#define W_IN_DRAG        2
#define W_IN_GROW        3
#define W_IN_GO_AWAY     4
#define W_IN_ZOOM_IN     5
#define W_IN_ZOOM_OUT    6

#define C_DRAW           0
#define C_TEST           1
#define C_CALC_RGNS      2
#define C_INIT           3
#define C_DISPOSE        4
#define C_POS            5
#define C_THUMB          6
#define C_DRAG           7
#define C_AUTO_TRACK     8

#define M_DRAW           0
#define M_CHOOSE         1
#define M_SIZE           2
#define M_POP_UP_RECT    3

#define MB_DRAW          0
#define MB_HIT           1
#define MB_CALC          2
#define MB_INIT          3
#define MB_SAVE          4
#define MB_RESTORE       5

typedef struct {
    int16_t top;
    int16_t left;
    int16_t bottom;
    int16_t right;
} DFRect;

typedef struct {
    int16_t v;
    int16_t h;
} DFPoint;

/* WDEF / CDEF / MDEF / MBDF Entry Points */
int32_t lc3_wdef_0(int16_t varCode, void *window, int16_t message, int32_t param);
int32_t lc3_wdef_1(int16_t varCode, void *window, int16_t message, int32_t param);
int32_t lc3_cdef_0(int16_t varCode, void *control, int16_t message, int32_t param);
int32_t lc3_cdef_1(int16_t varCode, void *control, int16_t message, int32_t param);
int32_t lc3_mdef_0(int16_t message, void *theMenu, DFRect *menuRect, DFPoint hitPt, int16_t *whichItem);
int32_t lc3_mbdf_0(int16_t message, int16_t param1, int32_t param2);

#endif /* LC3_DEFPROCS_H */
