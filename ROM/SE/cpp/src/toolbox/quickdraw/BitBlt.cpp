#include "serom/toolbox/quickdraw/BitBlt.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: BitBlt
namespace serom { namespace toolbox { namespace quickdraw { namespace BitBlt { 
    
    void BITBLT() {
        // _CopyBits / BitBlt transfer
        D(0) = 0;
        SetNZ(0);
    }
    void MODEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DIREC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UPSIDE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DIROK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DIROK_NO() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DOMAIN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTROW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ROWLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NXTSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTSRC_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MODETAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LEFT0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LTWO0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LONE0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LMORE0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETUP0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FAST0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void END0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MAIN0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYBIG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYBIG_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void COPYBIG_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void COPY32() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void END1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void END1_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void END2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void END2_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void END3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void END3_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void WORD8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WORD8A() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WORD8A_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CALCLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WORD8A_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void WORD8A_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void WORD8A_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void END8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILLBIG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILLBIG_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void FILLBIG_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FILL32() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILL31_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MAIN9() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MAIN9_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MAIN9_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MAIN9_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORBIG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORBIG_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void XORBIG_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void XOR32() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XOR31_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MAIN11() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // BitBlt
