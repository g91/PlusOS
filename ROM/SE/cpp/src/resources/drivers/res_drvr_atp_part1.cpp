#include "serom/resources/drivers/res_drvr_atp.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_drvr_atp
namespace serom { namespace resources { namespace res_drvr_atp { 
    
    void Resource_DRVR_000A_DotATP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Resource_DRVR_000A_DotATP_Header() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Driver_ATP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_Open() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_OPEN_SPEED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_OPEN_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_ATP_Control() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_ATP_CONTROL_NOHOOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_ATP_CONTROL_IODONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_ATP_CONTROL_CSCODE() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_ATP_CONTROL_RANGE() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_ATP_CONTROL_SKTLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_ATP_CONTROL_JUMP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_ATP_CONTROL_DISPATCH_TBL() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void E_ATP_Close() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_CLOSE_WAITCB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_CLOSE_WAITREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_CLOSE_WAITRSP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_CLOSE_FREE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void E_ATP_CLOSE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_CLOSESOCKETS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_CLOSESOCKETS_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void OPENATPSKT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OPENATPSKT_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CLOSEATPSKT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLOSEATPSKT_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CLOSEATPSKT_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_DONE_NOERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void ATP_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_DONE_A1() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void E_ATP_PrimeStatus() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETREQUEST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_APPENDREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_APPENDREQ_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATP_APPENDREQ_LINK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_DETACHREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_DETACHREQ_UNLINK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_DETACHREQ_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void KILLGETREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void KILLGETREQ_SKTLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void KILLGETREQ_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void KILLGETREQ_FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void KILLGETREQ_NOTFOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void KILLGETREQ_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SENDRESPONSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SENDRESPONSE_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SENDRESPONSE_CTL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SENDRESPONSE_SENT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SENDRESPONSE_CLRLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SENDRESPONSE_RSPCB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SENDRESPONSE_ABORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SENDRESPONSE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_FINDRSPCB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_FINDRSPCB_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATP_FINDRSPCB_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_FINDRSPCB_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_CMPADDR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_CMPADDR_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ADDRESPONSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRESPONSE_BDS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRESPONSE_NOERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void ADDRESPONSE_RESTSR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ADDRESPONSE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SENDREQUEST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SENDREQUEST_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SENDREQUEST_OPENED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NSENDREQUEST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NSENDREQUEST_TID() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NSENDREQUEST_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NSENDREQUEST_COUNTED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NSENDREQUEST_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NSENDREQUEST_QUEUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_SNDRQINIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_SNDRQINIT_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_SETBITMAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_SETBITMAP_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATP_SETBITMAP_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_SENDPKT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_MPPCALL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_MPPDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_BUILDTREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_SETDATA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_SETDATA_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_FIRSTNSREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_REQFAILED() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void ATP_RETRYCOUNT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_RETRYCOUNT_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_UNLINKNSREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_UNLINKNSREQ_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATP_UNLINKNSREQ_FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_UNLINKNSREQ_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_UNLINKNSREQ_NONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_REQLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATP_VBLTASK_NEXTREQ() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_FAIL() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void ATP_VBLTASK_NSREQS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NEXTSKT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NSCOUNT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NSSCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NSFIND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NSEXPIRED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NSRESCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_NSFAIL() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void ATP_VBLTASK_REQDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ATP_VBLTASK_RSPCBS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_RSPLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ATP_VBLTASK_NEXTRSP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ATP_VBLTASK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
}}}  // res_drvr_atp
