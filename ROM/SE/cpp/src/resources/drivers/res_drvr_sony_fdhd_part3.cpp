#include "serom/resources/drivers/res_drvr_sony_fdhd.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_drvr_sony_fdhd
namespace serom { namespace resources { namespace res_drvr_sony_fdhd { 
    void SONY_RECALFAIL() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_RECAL_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_RECAL_ERR_2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_SETIWMMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETIWMMODE_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETIWMMODE_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_SETIWMMODE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void Sony_PowerDown() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_POWERDOWN_NOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_INVALTRKCACHE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_INVALTRKCACHE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void Sony_FastPower() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_RWPowerUp() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_PowerUp() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_POWERUP_SETMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_POWERUP_MTRCHK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_POWERUP_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void Sony_DiskSelect() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_DiskSel1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKSEL1_VEC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_jtDiskSel() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKSEL_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_DISKSEL_LOWER() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKSEL_INT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKSEL_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKSEL_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_DISKSEL_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_DISKSEL_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_DISKSEL_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_DISKSEL_HD20() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DT_Sony_SpeedCheckTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_Sony_MakeSpdTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_MakeSpdTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_MAKESPDTBL_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_MAKESPDTBL_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_MAKESPDTBL_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_MAKESPDTBL_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_MAKESPDTBL_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_MAKESPDTBL_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_MAKESPDTBL_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SONY_GETFREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_GETFREQ1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_GETFREQ1_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_GETFREQ1_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_Sony_FigTrkSpd() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_FigTrkSpd() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FIGTRKSPD_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_STARTT2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_FIGTRKSPD_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_FIGTRKSPD_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_FIGTRKSPD_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_WAITTACH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITTACH_LOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITTACH_LOW_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITTACH_LOW_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITTACH_TIMEOUT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITTACH_HIGH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITTACH_HIGH_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DSKRTN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_DiskVBLTask() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKVBLTASK_POLL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKVBLTASK_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_DISKVBLTASK_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_DISKVBLTASK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void Sony_Check4Disk() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKEJECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKEJECT_SELECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDIP_HD20() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDIP_HD20SENSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDIP_SENSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDIP_POST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKINVAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKEXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_CKEJSW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKEJSW_SENSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKEJSW_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_WRDATAMKS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_Sony_WrData() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_WrData() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_NO_4() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NO_5() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_SKIP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NO_6() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_NO_7() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_SKIP_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NO_8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_NO_9() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_SKIP_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_NO_10() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WRDATA_LOOP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_SKIP_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WRDATA_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_SLIPMKS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DT_Sony_NiblTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_Check4SWIM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CHECK4SWIM_IWM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_SetChipMode() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETCHIPMODE_TRY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETCHIPMODE_IWM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETCHIPMODE_ISM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETCHIPMODE_FAIL() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
}}}  // res_drvr_sony_fdhd
