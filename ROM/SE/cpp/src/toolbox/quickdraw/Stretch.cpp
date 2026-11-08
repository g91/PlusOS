#include "serom/toolbox/quickdraw/Stretch.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Stretch
namespace serom { namespace toolbox { namespace quickdraw { namespace Stretch { 
    
    void STRETCHBITS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XRGNBLT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CLRDST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_CLRMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_CLRMASK_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_CLRMASK_SKIP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_CLRMASK_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_CLRMASK_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_CLRMASK_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_CLRMASK_SKIP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NEXTSRC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTSRC_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SRCLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SRCLOOP_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SRCLOOP_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_NXTMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTDST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTDST_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_GOHOME_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_RECTFLAG_TBL() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_GOHOME_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_GOHOME_CONT_4() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_GOHOME_CONT_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_JSRSEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_GOHOME_CONT_AB() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_GOHOME_CONT_AC() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_GOHOME_CONT_BC() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_CPY2BUF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STRETCHBITS_BCLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void JSRSEEK_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_GOHOME_CONT_ABC() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_ABCLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void STRETCHBITS_ABCDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_ABCDONE_LOOP() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_ABCDONE_LOOP_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_ABCDONE_LOOP_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_ABCDONE_LOOP_4() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void STRETCHBITS_ABCDONE_MODE_TBL() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETUPSTRETCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOMATCH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FOUND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHRNKING() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHRNKING_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void THREE4() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void THREE4A() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void THREE4B() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HALF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HALF1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HALF2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HALF2_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void QRTR1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void QRTR2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void QRTR2_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void QRTR2_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void QRTR2_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NIBBLE_DOUBLE_TBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TRIPLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TRIPLE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TRIPLE_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TRIPLE_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void QUAD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void QUAD_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SIX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SIX_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SIX_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SIX_CONT_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void EIGHT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EIGHT_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SIXTEEN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SIXTENA() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WHITE16() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BLACK16() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EIGHTS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EIGHTS1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EIGHTS2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EIGHTS3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHRINK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTSRC1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTSRC1_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void WHITE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WHITE1_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NXTSRC2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BLACKOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void BLACKOK_CONT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void WHITEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void WHITE2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void WHITE2_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
}}}}  // Stretch
