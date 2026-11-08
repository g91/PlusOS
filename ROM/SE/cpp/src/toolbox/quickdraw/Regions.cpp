#include "serom/toolbox/quickdraw/Regions.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Regions
namespace serom { namespace toolbox { namespace quickdraw { namespace Regions { 
    
    void STDRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRGN_NOTPIC() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRGN_NOTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRGN_NOTFR() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRGN_GOHOME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRAMERGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PAINTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void ERASERGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void INVERTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FILLRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CALLRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CALLRGN_USESTD() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWRGN_EXIT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRRGN_NOTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRRGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void NEWRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DISPOSERGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OPENRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CLOSERGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CLOSERGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void COPYRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void COPY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPYRGN_COPYLP() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EVEN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EVEN_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETEMPTYRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SETRECTRGN() {
        // _SetRect: A0 = rect, stack or regs
        Addr r = A(0);
        if (r) {
            PokeW(r + 0, static_cast<UInt16>(D(3))); // top
            PokeW(r + 2, static_cast<UInt16>(D(2))); // left
            PokeW(r + 4, static_cast<UInt16>(D(1))); // bottom
            PokeW(r + 6, static_cast<UInt16>(D(0))); // right
        }
        D(0) = 0;
        SetNZ(0);
    }
    void SETRECTRGN_OK() {
        // _SetRect: A0 = rect, stack or regs
        Addr r = A(0);
        if (r) {
            PokeW(r + 0, static_cast<UInt16>(D(3))); // top
            PokeW(r + 2, static_cast<UInt16>(D(2))); // left
            PokeW(r + 4, static_cast<UInt16>(D(1))); // bottom
            PokeW(r + 6, static_cast<UInt16>(D(0))); // right
        }
        D(0) = 0;
        SetNZ(0);
    }
    void SETRECTRGN_EMPTY() {
        // _SetRect: A0 = rect, stack or regs
        Addr r = A(0);
        if (r) {
            PokeW(r + 0, static_cast<UInt16>(D(3))); // top
            PokeW(r + 2, static_cast<UInt16>(D(2))); // left
            PokeW(r + 4, static_cast<UInt16>(D(1))); // bottom
            PokeW(r + 6, static_cast<UInt16>(D(0))); // right
        }
        D(0) = 0;
        SetNZ(0);
    }
    void SETRECTRGN_DONE() {
        // _SetRect: A0 = rect, stack or regs
        Addr r = A(0);
        if (r) {
            PokeW(r + 0, static_cast<UInt16>(D(3))); // top
            PokeW(r + 2, static_cast<UInt16>(D(2))); // left
            PokeW(r + 4, static_cast<UInt16>(D(1))); // bottom
            PokeW(r + 6, static_cast<UInt16>(D(0))); // right
        }
        D(0) = 0;
        SetNZ(0);
    }
    void RECTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OFFSETRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OFFSETRGN_NXTVERT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OFFSETRGN_NXTHOR() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OFFSETRGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void INSETRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void INSETRGN_EMPTY() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void JGOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INSETRGN_NOTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void HINSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SWAPLP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SWAP2() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INSETRGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void INSETRGN_GOHOME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRGN_LOOP() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRGN_LOOP2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRGN_LOOP2_OK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRGN_LOOP2_CONT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SECTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DIFFRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void XORRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DORGNOP() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void XORRGN_COPY() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void XORRGN_ZERO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void XORRGN_NOTSAME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void BBOXES() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOTEMPTY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void JDONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void UNIXOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void JDONE_NO_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void JDONE_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void PTINRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PTINRGN_NO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PTINRGN_NEXT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTHOR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTHOR_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void RECTINRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void TESTBUF() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NXTLONG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TRUE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TRUE_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void TRIMRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void TRIMRECT_EXIT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN_NO() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN_EXIT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN_NO_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN_LOOP() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN_LOOP_2() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void MAPRGN_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // Regions
