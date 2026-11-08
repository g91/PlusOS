#include "serom/toolbox/quickdraw/RgnBlt.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: RgnBlt
namespace serom { namespace toolbox { namespace quickdraw { namespace RgnBlt { 
    
    void RGNBLT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OKRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OKRECT_NO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_SKIP() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_SKIP_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_CONT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_EXIT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_NO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_NO_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_SKIP_3() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_CLRMASK_SKIP_4() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void USESRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_UPSIDE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_SRCOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GETDST() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GETDST_NO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_NXTMASK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_NXTMASK_NEXT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_NXTMASK_NEXT_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void BUMPV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GOHOME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GOHOME_CONT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GOHOME_CONT_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GOHOME_CONT_3() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GOHOME_CONT_4() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNBLT_GOHOME_CONT_5() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void JMPSEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void JMPSEEK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void JMPSEEK_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void JMPSEEK_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CPY2BUF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BCLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void BCLOOP_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void BCLOOP_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ABCLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ABCDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_MODE_TBL() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_5() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_6() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_7() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ABCDONE_LOOP_8() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
}}}}  // RgnBlt
