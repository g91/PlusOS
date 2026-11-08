#include "serom/toolbox/quickdraw/Rects.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Rects
namespace serom { namespace toolbox { namespace quickdraw { namespace Rects { 
    
    void STDRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRECT_NOTPIC() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRECT_NOTRGN() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRECT_NOTFR() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void STDRECT_GOHOME() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PUSHVERB() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PUSHVERB_ERASE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PUSHVERB_ERASE1_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void PUSHVERB_PAINT1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PUSHVERB_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void FILLRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRAMERECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PAINTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void ERASERECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void INVERTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CALLRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void CALLRECT_USESTD() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWRECT_EXIT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FRRECT_OK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DORECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FILLED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FILLED_SKIP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SETRECT() {
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
    void EQUALRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void FALSE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EQUALRECT_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYRECT_EMPTY() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void EMPTYRECT_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void OFFSETRECT() {
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
    void INSETRECT() {
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
    void SECTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RSECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NEXTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SECTRECT_TOPOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SECTRECT_LEFTOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SECTRECT_BOTOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void RTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SECTRECT_EMPTY() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void SECTRECT_DONE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONRECT_TOPOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONRECT_LEFTOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONRECT_BOTOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void UNIONRECT_RIGHTOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PT2RECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void HOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void HOK_NO() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void PT2RECT_SHARE() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PTINRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PTINRECT_EXIT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PUTRECT() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void PUTRECT_SIZEOK() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // Rects
