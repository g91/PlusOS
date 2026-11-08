#include "serom/extra/rom_tail.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: rom_tail
namespace serom { namespace extra { namespace rom_tail { 
    
    void ROMTail_Initials_SE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ROMTail_BuildDate_Thu_Nov_20_1986() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ROMTail_Initials_FDHD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FDHD_ResourceTailPatchCode() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_RDADDR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_RDDATA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_SETUPPOLL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_CTLFORMAT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_CTLVERIFY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_RDADDRSETUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_READDATA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_TOEMPTYPD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_E_SETUPPOLL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_DRIVEICON() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_MEDIAICON() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_DRVINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_SETDRVTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAILENT_RAMTEST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_ADRDISK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_GETDRV1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_MRDADDR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_DSKRTN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_MRDDATA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_SETCHIPMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_DISKSELECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_ADRANDSENSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_MFMTTRACK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_DSKRWOFF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_MVERTRACK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_EMPTYPD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_SEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_RWPOWERUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TAIL_DRV_GETFREQ1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DT_Sony_SectorAdrTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_Sony_SetUpPoll() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_SetUpPoll() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_RdAddrSetup() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDADDRSETUP_SIDE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_Sony_RdAddr() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDADDR_GCR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_RdAddr() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDADDR_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDADDR_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_SKIP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_CONT_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_LOOP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDR_SKIP_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDADDRXIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDADDR_ERR_2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_RDADDR_ERR_3() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_DTAMKS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_Sony_RdData() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDDATA_GCR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_RdData() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RDDATA_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_LOOP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_LOOP_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_8() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_9() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_RDDATA_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_RDDATA_SKIP_8() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_10() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_11() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_9() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_ERR_2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_RDDATA_ERR_3() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_RDDATA_CONT_12() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_LOOP_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_SKIP_10() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_CONT_13() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RDDATA_ERR_4() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_DENIBLTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_ADRMKTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_CtlFormat() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_CHKFMT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_SETCHIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_SELECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_SIDES() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_BUILD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_MARKS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_MARKS_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_TRACKLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_CTLFORMAT_GCR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLFORMAT_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FMTVEREXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_FMTSPEEDTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SPDSEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SPDSEEK_CKDRIFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SPDSEEK_CHECK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SPDSEEK_ADJUST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SPDSEEK_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_SPDSEEK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_SPDSEEK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_FORMATTRACK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FORMATTRACK_1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FORMATTRACK_CHECK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FORMATTRACK_GAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FORMATTRACK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_FORMATTRACK_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_FORMATTRACK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_FORMATTRACK_DECRSYNC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FORMATTRACK_DECRSN1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_SIDE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_STRTSYNC() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_DOFORMAT_WSS1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_WSS1_HSHK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_WSS1_HSHK2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_NXTSECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_GAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_GAP_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_GAP_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_ADRMK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_ADRMK_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_DATA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_DATA_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_SLIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DOFORMAT_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_DOFORMAT_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_FILLINMARKS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FILLINMARKS_1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FILLINMARKS_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void Sony_CtlVerify() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLVERIFY_START() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLVERIFY_CLASS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLVERIFY_TRACK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLVERIFY_GCR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLVERIFY_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTLVERIFY_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_VERTRACK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_VERTRACK_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_VERTRACK_1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_VERTRACK_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_VERTRACK_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_VERTRACK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_VERTRACK_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_VERTRACK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_TOEMPTYPD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FMTVERSEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SYNCCALL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SYNCCALL_WAIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SYNCCALL_CALL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FVPOWERUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRVTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_DISKETTE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_EXTDRIVE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_LOWERDRIVE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ICON_UPPERDRIVE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_MEDIAICON() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_MEDIAICON_ADD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_DRIVEICON() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_DRIVEICON_ADD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_DRVINFO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_DRVINFO_NEW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_PARAM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_ICONRET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_GETDRVTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETDRVTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_TAIL_DISKDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FDHD_RAMTestPatch() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FDHD_RAMTESTPATCH_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ROMTail_Initials_FDHD_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ROMTail_BuildDate_Tue_Oct_11_1988() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MacSEFDHD_ROM_End() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // rom_tail
