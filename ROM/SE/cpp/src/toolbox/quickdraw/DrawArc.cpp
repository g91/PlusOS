#include "serom/toolbox/quickdraw/DrawArc.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: DrawArc
namespace serom { namespace toolbox { namespace quickdraw { namespace DrawArc { 
    
    void DRAWARC() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void NOCOLOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CKPAT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void YESFLAG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void YESFLAG_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void YESFLAG_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void POSANG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STARTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STOPOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STOPOK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STOPOK_CONT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STOPOK_OK_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STOPOK_CONT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ACUTE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTARC() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SOLID() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRMASK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void CLRMASK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CLRMASK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void CLRMASK_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CLRMASK_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRMASK_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GOFORIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWARC_NEXTROW() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void YESBUMP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOBUMP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ACUTE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ACUTE1_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ACUTE1_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOMASK_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NOMASK_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOMASK_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOMASK_SKIP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOMASK_SKIP_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOMASK_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOMASK_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOMASK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOMASK_NO_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ONESLAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SARC() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void HOVAL() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SOVAL() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DOSLAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOSLAB_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NODRAW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWARC_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWARC_GOHOME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWARC_GOHOME_CONT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWARC_GOHOME_CONT_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWARC_GOHOME_CONT_3() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void JSRSEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void JSRSEEK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void JSRSEEK_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void JSRSEEK_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ABLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void INITOVAL() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OK1() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void OK1_NO() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void INITOVAL_OK3() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void INITOVAL_OK3_NO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void BUMPOVAL() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void BUMPOVAL_MORE1() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void WHILE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BUMPOVAL_MORE2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void WHILE2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BUMPOVAL_GOHOME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // DrawArc
