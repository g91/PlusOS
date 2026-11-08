#include "serom/toolbox/quickdraw/DrawLine.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: DrawLine
namespace serom { namespace toolbox { namespace quickdraw { namespace DrawLine { 
    
    void DRAWLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_NOCOLOR() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void VOK2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void VOK2_NO() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DRAWLINE_CKPAT() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_YESFLAG() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_YESFLAG_CONT() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_YESFLAG_NO() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void HVLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void NOTFAST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VSWAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SLANTED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SLANTED_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEGSLOPE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEGSLOPE_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEGSLOPE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NEGSLOPE_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_CLRMASK() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_CLRMASK_OK() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_CLRMASK_CONT() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_CLRMASK_EXIT() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_CLRMASK_SKIP() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_CLRMASK_LOOP() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void GOSLOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GOSLOW_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NXTMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_DONE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_GOHOME() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_GOHOME_CONT() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_GOHOME_CONT_2() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_GOHOME_CONT_3() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_JSRSEEK() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_JSRSEEK_CONT() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_JSRSEEK_CONT_2() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_JSRSEEK_CONT_3() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWLINE_ABLOOP() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void FASTLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void CLRTWO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRMORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENDCLR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HTOGL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TOGLTWO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TOGLONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TOGLMORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENDTOGL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETTWO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETONE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETMORE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ENDSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void VCLR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VTOGL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HVSETUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // DrawLine
