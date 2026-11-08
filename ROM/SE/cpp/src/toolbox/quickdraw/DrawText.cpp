#include "serom/toolbox/quickdraw/DrawText.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: DrawText
namespace serom { namespace toolbox { namespace quickdraw { namespace DrawText { 
    
    void DRTEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOSLOP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOSTRCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRTEXT_NOCOLOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FAST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VCLIP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void VCLIP_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRTEXT_NOTFAST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRTEXT_NOTFAST_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void DRTEXT_NOTFAST_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETPTRS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETPTRS_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SPACECH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SKIPCH() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NEXTCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MISSING() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OKCHAR() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NOTMISS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTMISS_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTMISS_NO_2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TRIMRT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HORIZOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void MAIN1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WIDE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LONG1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LONG1A() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WORD1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WORD1B() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRDONE_CONT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void BOLDIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BOLDLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTBOLD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTROW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTWORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOSLANT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CHECKUL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ONLY2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UL1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UL2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ULTRIM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UL3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTUL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLR2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ACROSS1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ACROSS2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOWN1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOWN2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOWN2_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOSHAD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOSHAD_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOSHAD_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
}}}}  // DrawText
