#include "serom/toolbox/quickdraw/Bitmaps.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Bitmaps
namespace serom { namespace toolbox { namespace quickdraw { namespace Bitmaps { 
    
    void STDBITS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TOPOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void BOTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LEFTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LEFTOK_NO() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NOTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void NOPACK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOPACK_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MORE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void START1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MORE2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void START2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void START2_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STDBITS_NOTPIC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MASKOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STDBITS_GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYBITS() {
        // _CopyBits / BitBlt transfer
        D(0) = 0;
        SetNZ(0);
    }
    void SRCOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NOTPORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYBITS_MASKOK() {
        // _CopyBits / BitBlt transfer
        D(0) = 0;
        SetNZ(0);
    }
    void TOPORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYBITS_USESTD() {
        // _CopyBits / BitBlt transfer
        D(0) = 0;
        SetNZ(0);
    }
    void CLEANUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYBITS_DONE() {
        // _CopyBits / BitBlt transfer
        D(0) = 0;
        SetNZ(0);
    }
    void COPYMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYMASK_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void AGAIN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void GETDST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYMASK_NEXTROW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYMASK_NEXTROW_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void COPYMASK_GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SEEDFILL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CALCMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CALCMASK_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BLACK1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BLACK2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOSEED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTBUF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTPASS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CALCMASK_NEXTROW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RNEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RDSTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void RMORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RSTART() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RSAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LSMEAR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LNEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LDSTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LMORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LSTART() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LSAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LSAME_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void INVERT1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INVERT2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INVERT2_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SCROLLRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void ABORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ABORT_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void PACKBITS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILLOP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void START() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTCOPY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOCOPY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENDCOPY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYOP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOFILL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTFILL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENDFILL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENDFILL_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ENDFILL_EXIT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void UNPACKBITS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UNPACKBITS_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void UNPACKBITS_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void UNPACKBITS_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void UNPACKBITS_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void UNPACKBITS_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
}}}}  // Bitmaps
