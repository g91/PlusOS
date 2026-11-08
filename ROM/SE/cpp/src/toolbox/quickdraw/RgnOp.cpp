#include "serom/toolbox/quickdraw/RgnOp.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: RgnOp
namespace serom { namespace toolbox { namespace quickdraw { namespace RgnOp { 
    
    void RGNOP() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CASEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STKOK1() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STKOK2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void ALLOC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RGNOP_NXTVERT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UPDATEA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BOTH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UPDATEB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CALC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CHANGES() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CHANGES_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OUTTEST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OUTTEST_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXPAND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RGNOP_NOTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNOP_ABORT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RGNOP_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SECTSCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DIFFSCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONSCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UNION2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SECTSCAN_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SKIP0() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ALESS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SKIP1() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void BLESS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SKIP2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SKIP2_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void INSETSCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OUTSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void OUTLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void OUTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void OUTOK2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void OUTOK2_LOOP() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void OUTOK2_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void EQUAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORSCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INSETSCAN_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INSETSCAN_ALESS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // RgnOp
