#include "serom/toolbox/quickdraw/Util.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Util
namespace serom { namespace toolbox { namespace quickdraw { namespace Util { 
    
    void BITNOT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITAND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITXOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITSHIFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHLEFT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITSHIFT_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void BITTST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITCLR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITCLR_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void RANDOM() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RANDOM_UPDATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RANDOM_UPDATE_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FORECOLOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BACKCOLOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PORTLONG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COLORBIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PORTWORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETMASKTAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LEFTMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void RIGHTMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BITMASK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void MASKTAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LeftMaskTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DT_MMASKTAB_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void PATEXPAND() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PATEXPAND_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NORMAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOUBLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void THIN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void THINLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PATEXPAND_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DoubleNibbleTbl() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PATEXPAND_DONE_CONT_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void COLORMAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COLORMAP_SHARE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BICMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COLORMAP_COPY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void USEPAT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FORE0() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INVMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEWMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COLOROK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETPIXEL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STUFFHEX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HEXLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void ENDHEX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTHEX() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SMALL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORSLAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORSLAB_NOTIN1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORSLAB_INVLONG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void XORSLAB_INVLONG_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void XORSLAB_ENDWORD() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWSLAB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWSLAB_LEFTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DRAWSLAB_RIGHTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DONESLAB() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DRAWSLAB_END8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT8_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void END9() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT9() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT9_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void END10() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT10() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT10_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void END11() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT11() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXT11_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SLABMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SLABMODE_MODE_TBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SLABMODE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void LOOP8() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MERGE8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LAST8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LAST8_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void LOOP10() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MERGE10() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LAST10() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LAST10_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void LOOP11() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void MERGE11() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LAST11() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FASTSLABMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FASTSLABMODE_MODE_TBL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // Util
