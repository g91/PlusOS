/* MemoryMgr_glue.c - Memory Manager calls that were library glue, not traps. */
#include <plusrom/MemoryMgr.h>
#include <plusrom/boot.h>

/* InitZone: the trap ($A019) takes A0 -> {startPtr, limitPtr, cMoreMasters, pGrowZone}. */
pascal void InitZone(GrowZoneUPP pGrowZone, int16_t cMoreMasters, Ptr limitPtr, THz startPtr)
{
    plusrom_init_zone(startPtr, limitPtr, cMoreMasters, (void *)pGrowZone);
}
