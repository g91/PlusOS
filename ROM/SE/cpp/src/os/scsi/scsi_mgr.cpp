#include "serom/os/scsi/scsi_mgr.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: scsi_mgr
namespace serom { namespace os { namespace scsi_mgr { 
    
    void SCSISTUFF() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void INITSCSIMGR() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_INIT_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_PRAM_OK() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_INIT_RTS() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSIMGR() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_DISPATCH_OFFSETS() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIunimplemented() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_UNIMPL_HANG() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIInstall() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIStat() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIReset() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_RETURN_OK() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIGet() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_GET_WAIT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_GET_TRY() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_GET_STORE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSISelect_SelAtn() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEL_GO() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEL_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSICmd() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CMD_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIWrite() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIRead() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_READ_LOAD_TIB() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_TIB_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scInc() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scNoInc() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scAdd() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scMove() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scLoop() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scStop() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scComp() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSI_scNOp() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_TIB_EXIT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIComplete() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CMPL_POLL() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CMPL_CHKTMO() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CMPL_GOT_STATUS() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CMPL_GOT_MSG() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CMPL_RESULT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIMsgIn() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_MSGIN_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void P_SCSIMsgOut() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_PROLOGUE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_EPILOGUE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ARBITRATE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ARB_TRY() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ARB_WAIT_AIP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ARB_GOT_AIP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_DO_SELECT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEL_NO_ATN() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEL_FAIL() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEND_CDB() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CDB_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CDB_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_ENGINE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_WRITE_WAIT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_WAIT_REQ() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_PHASE_CHK() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_DISPATCH() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_RW_DEC() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_C_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_C_NEXT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_C_PHCHK() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_R_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_R_NEXT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_R_PHCHK() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_R_PHERR() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_W_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_W_NEXT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_W_PHCHK() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_BLIND() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_BLIND_WAIT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_BLIND_R() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_BLIND_W() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_DRAIN() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_DRAIN_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_CLR() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_RESTORE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_XFER_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BUSERR_HANDLER() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_CMP_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_COMPARE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_READ() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_READ_ALIGNED() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_READ_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_READ_TAIL_BODY() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_READ_TAIL_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_WRITE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_WRITE_ALIGNED() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_WRITE_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_WRITE_TAIL_BODY() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BLIND_WRITE_TAIL_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_PREP_READ() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_PREP_PHASE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_PREP_WRITE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_READ_STATUS() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_READ_STATUS_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_READ_MSGIN() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_READ_MSGIN_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEND_MSGOUT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_SEND_MSGOUT_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ARB_DELAY() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ARB_DELAY_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_ASSERT_RST() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_RST_HOLD() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_RST_RECOVER() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_BSY() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_BSY_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_BSY_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_BSY_DROP_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_BSY_DROP_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_REQ_DROP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_REQ_DROP_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_REQ_DROP_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_REQ() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_REQ_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_WAIT_REQ_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_HANDLE_BAD_PHASE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BAD_PHASE_LOOP() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BAD_PHASE_HAVE_REQ() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BAD_PHASE_DRAIN() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BAD_PHASE_OUT() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BAD_PHASE_HANDSHAKE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_BAD_PHASE_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CHECK_PHASE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
    void SCSI_CHECK_PHASE_DONE() {
        // SCSI Manager routine
        D(0) = 0;
        SetNZ(0);
    }
}}}  // scsi_mgr
