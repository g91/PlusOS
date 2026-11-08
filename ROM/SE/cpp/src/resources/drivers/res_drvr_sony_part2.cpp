#include "serom/resources/drivers/res_drvr_sony.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_drvr_sony
namespace serom { namespace resources { namespace res_drvr_sony { 
    void SONY_DORECAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RESEEK_ERR_7() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DRVVAROFS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_GetRDrive() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_GetDrive() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_GetDrv1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_GetDrv2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_GETDRV3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_AdrDisk() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_ADRDISK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_ADRDISK_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_ADRDISK_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_ADRDISK_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_ADRDISK_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_ADRDISK_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void Sony_AdrAndSense() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_Sense() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_AdrAndStrb() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_Pulse() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_CkDCDExist() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDCDEXIST_NONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CKDCDEXIST_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void Sony_SetSpeed() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETSPEED_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETSPEED_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_SETSPEED_OK_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_SETSPEED_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETSPEED_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETSPEED_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_LFSR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SETSPEED_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_LOOP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_CONT_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SETSPEED_CONT_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_Sony_Seek() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_Seek() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SEEKLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SEEK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SEEK_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SEEK_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_SEEKEND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_SEEKEND1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAIT100() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAIT100_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_CHECKNEW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CHECKNEW_WAIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CHECKNEW_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_WAITREADY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITREADY_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_WAITREADY_SENSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAITREADY_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_Sony_WakeUp() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_WakeUp() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAKEUP_GO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAKEUP_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_WAKEUP_RET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAKESETUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAKESETUP_1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_WAKESETUP_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_GOWAKEIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_GOWAKEIT_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_Sony_Recal() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Sony_Recal() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_RECAL_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RECAL_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RECAL_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SONY_RECAL_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SONY_RECALFAIL() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
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
    void Sony_jtDiskSel() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_DISKSEL_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
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
}}}  // res_drvr_sony
