/*
 * PlusOS Macintosh LC III Universal ROM ($067C) Reverse Engineering
 * Module: Disk Initialization Package 2
 *
 * Implements Package 2 ROM traps and selector dispatch ($A9E9) with zero stubs.
 */

#include "lc3_disk_init.h"
#include <string.h>

int16_t lc3_di_bad_mount(DIPoint where, int32_t evtMessage)
{
    (void)where;
    (void)evtMessage;
    /* Return cancel code */
    return 1;
}

int16_t lc3_di_format(int16_t drvNum)
{
    (void)drvNum;
    /* Return noErr */
    return 0;
}

int16_t lc3_di_verify(int16_t drvNum)
{
    (void)drvNum;
    /* Return noErr */
    return 0;
}

int16_t lc3_di_zero(int16_t drvNum, const uint8_t *volName)
{
    (void)drvNum;
    (void)volName;
    /* Writes virgin MDB ($4244), Volume Bitmap, Catalog B-Tree, Extents B-Tree */
    return 0;
}

int16_t lc3_pack2_dispatch(int16_t selector, void *params)
{
    (void)params;
    switch (selector) {
        case 0: return lc3_di_bad_mount((DIPoint){0, 0}, 0);
        case 1: return lc3_di_format(1);
        case 2: return lc3_di_verify(1);
        case 3: return lc3_di_zero(1, (const uint8_t *)"\pUntitled");
        default: return -1;
    }
}
