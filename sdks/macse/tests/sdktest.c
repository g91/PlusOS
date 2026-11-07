/* sdktest.c - compile check for the Macintosh SE SDK: every header, plus calls
 * through each calling convention (Pascal stack trap, register trap,
 * selector dispatch, inline asm), low memory, hardware, glue. */
#include <serom/serom.h>
#include <plusos/plusos.h>
#include <setjmp.h>
#include <assert.h>

int sdk_compile_test(void)
{
    Rect r = { 10, 10, 50, 100 };
    FrameRect(&r);                              /* Toolbox, Pascal stack */
    Ptr p = NewPtrClear(128);                   /* OS, register (#pragma parameter) */
    DisposePtr(p);
    uint8_t km[16];
    GetKeys(km);
    CInfoPBRec ci = { 0 };
    (void)PBGetCatInfoSync(&ci);                /* HFS selector dispatch */
    long t = TickCount();
    Delay(1, 0);                                /* inline-asm register call */
    int in_rom = serom_trap_in_rom(SEROM_TRAP_FrameRect);
    uint32_t mt = (uint32_t)LM_MemTop;          /* low-memory global (typed: Ptr) */
    uint8_t via = HW_VIA_PortB;                 /* hardware register */
    int16_t rc = serom_init_fs(10);
    INTEGER ref; LONGINT n = 4; char buf[4];
    FSOpen((ConstStringPtr)"\pREADME", 0, &ref);   /* SDK glue */
    FSRead(ref, &n, buf);
    FSClose(ref);
    char s[32];
    snprintf(s, sizeof s, "%d %lu %x %d", in_rom, (unsigned long)mt, via, serom_machine());
    return (int)t + rc + SYS_ioRefNum + (int)(uintptr_t)serom_tool_table();
}

#ifdef ADB_TEST
/* ADB Manager: in the Mac SE ROM, not the Mac Plus ROM */
int sdk_adb_test(void)
{
    ADBReInit();
    return CountADBs();
}
#endif

#ifdef EXPECT_UNAVAILABLE
/* must fail to compile: in neither ROM */
void sdk_unavailable_test(void)
{
    EventRecord e;
    WaitNextEvent(everyEvent, &e, 0, 0);
    long resp; Gestalt(0, &resp);
}
#endif
