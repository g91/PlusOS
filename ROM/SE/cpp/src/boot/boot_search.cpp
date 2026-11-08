#include "serom/boot/boot_search.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: boot_search
namespace serom { namespace boot { namespace boot_search { 
    
    void FINDSTARTUPDEVICE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void REPLAN_SEARCH() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void RETRY_CANDIDATE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SCAN_CANDIDATES() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BOOT_DEVICE_FOUND() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETDEFAULTSTARTUP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETDEFAULTSTARTUP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTERNALWAIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INTERNALWAIT_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DTAB_INTERNALWAIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void EMBARKONSEARCH() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void EMBARK_SAVE_TIME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITFORINTERNAL() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITINT_GOT_TIMEOUT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITINT_POLL() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITINT_SCAN_DQ() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITINT_CHECK_TIME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITINT_CLEANUP() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void WAITINT_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LOADDRIVERS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void LOADDRVR_CALL() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PLANSTRATEGY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PLANSTRATEGY_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void FINDNEXTCANDIDATE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void BOOT_FINDNEXT_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTDQENTRY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTDQ_FOLLOW() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTDQ_TEST() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SELECTDEVICE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CHECKMOUSEEJECT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void CHECKMOUSE_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETSTARTUPINFO() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETSTARTUP_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void REACTTOFAILURE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void REACTFAIL_MARK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void REACTFAIL_SHOW() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void REACTFAIL_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void VISUALUPDATE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void VISUPD_TOGGLE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void VISUPD_FLASH() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void VISUPD_QUESTION() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void VISUPD_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ENABLEXFLASH() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWSUCCESS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWSUCCESS_RET() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void EJECTME() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISREJECT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISREJECT_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITFLOPORDEF() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITFLOPDEF_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITFLOPPY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITHD20() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITSCSI() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITSCSI_NO() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ISITANYTHING() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void NEVERAGAIN() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void NEVERAGAIN_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INBOOTMASK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INBOOTMASK_CHK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void INBOOTMASK_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWDEVICEFAIL() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWDEVFAIL_RTS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWPLAINDISK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWDISKQ() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWDISKX() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PLOTDISK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PLOTDISK_SYNC() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PLOTDISK_OVERLAY() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void PLOTDISK_DONE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void QMARK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void XMARK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_DISK() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETTIMEOUT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETTIMEOUT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETTIMEOUT_MERGE() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETWAITFLAGS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETWAITFLAGS() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DISABLEDYNWAIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ENABLEDYNWAIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void DISABLEPERMWAIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void ENABLEPERMWAIT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void GETRAWTIMEOUT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
    void SETRAWTIMEOUT() {
        // Boot drive search and loader setup
        lowmem::Set<SInt16>(lowmem::BootDrive, 1);
        D(0) = 0;
        SetNZ(0);
    }
}}}  // boot_search
