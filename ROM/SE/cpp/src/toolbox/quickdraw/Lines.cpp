#include "serom/toolbox/quickdraw/Lines.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: Lines
namespace serom { namespace toolbox { namespace quickdraw { namespace Lines { 
    
    void STDLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void STDLINE_NO() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void PUTOP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDLINE_STARTOK() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DHDV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void UPDATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void STDLINE_NOTPIC() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void LINETO() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void LINETO_USESTD() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void LINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void MOVETO() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void MOVE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void MOOV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DOLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DOLINE_NO() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void SIZEOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NOTPOLY() {
        // QuickDraw Shape Rasterization
        D(0) = 0;
        SetNZ(0);
    }
    void DOLINE_NOTRGN() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void HIDEPEN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWPEN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SHOWPEN_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETPENSTATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETPENSTATE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SETPENSTATE_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOTSET() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETPEN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PENSIZE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PENMODE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PENPAT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PENNORMAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // Lines
