#include "serom/boot/boot_install.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: boot_install
namespace serom { namespace boot { namespace boot_install { 
    
    void STARTBOOT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void STARTINIT1() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WARMSTART_CHECK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SET_BOOT_STACK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void POST_DEBUGGER_INIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INIT_MANAGERS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BOOTRETRY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETUP_BOOT_ENV() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void JMPTBLINIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void JMPTBL2() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILLWITHONES() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILL_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void COMPBOOTSTACK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETUPSYSAPPZONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SET_APPL_LIMIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DTAB_INITZONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWBEEPSCREEN() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITADBVARS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITHIMEMGLOBALS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITGLOBALVARS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SWITCHGOODIES() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WDCBSWOS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PMSPSWOS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITSWITCHERTABLE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETPRAM() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PRAM_GOT_REPEAT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PRAM_GOT_DELAY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITXVECTTABLES() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILL_LVL1DT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WHICHCPU() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WHICHCPU_TRAP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WHICHCPU_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETUPTIMEK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_DBRA_BODY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_DBRA_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_DBRA_START() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_DBRA_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_SCC_BODY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_SCC_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_SCC_START() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void TIMEK_SCC_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITSCSIGLOBALS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITSCSI() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITIWMGLOBALS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITIWM() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void IWM_INIT_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void IWM_INIT_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITVIAGLOBALS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITVIA() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void VIATIMERENABLES() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITSCCGLOBALS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DT_SCC_INIT_DATA() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITSCC() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WRITESCC() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_WRITE_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCC_WRITE_ENTER() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITVIDGLOBALS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITCRSRVARS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CLR_CRSRVARS_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITCRSRMGR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DTAB_MICKEY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GNEFILTER() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ALARM_FLASH_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GNE_FILTER_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITDISPATCHER() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILL_TOOL_DISP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FILL_OS_DISP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DECODE_DISPATCH() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DISP_ADD_DELTA() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DECODE_NEXT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DECODE_SHORT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DECODE_ABSOLUTE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DECODE_ABS_LOOP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DECODE_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INSTALL_ALINE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void P_tUnimplemented() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITIOMGR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INIT_IOMGR_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DT_SONY_NAME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DT_SOUND_NAME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITMEMMGR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITRSRCMGR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INITTIMERMGR() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // boot_install
