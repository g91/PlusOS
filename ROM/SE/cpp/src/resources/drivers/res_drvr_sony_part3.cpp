#include "serom/resources/drivers/res_drvr_sony.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_drvr_sony
namespace serom { namespace resources { namespace res_drvr_sony { 
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
    void SONY_CKDIP_EVENT() {
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
    void SONY_CKEJSW_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DRVTBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_MEDIAICON() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_DRIVEICON() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SONY_CTL_DRVINFO() {
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
    void SONY_GETDRVTBL_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
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
}}}  // res_drvr_sony
