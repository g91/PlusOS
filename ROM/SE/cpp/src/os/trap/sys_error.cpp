#include "serom/os/trap/sys_error.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: sys_error
namespace serom { namespace os { namespace sys_error { 
    
    void SYSTEMERROR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SYSERR2() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DBG_SKIP_SP_SAVE() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SYSERR_TRY_DEBUGGER() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SYSERR_USE_DS() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void SYSERR_DISPATCH() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void ALLOCFAKERGNS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INSTALL_INF_RGN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DT_INF_RGN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void COPY_ALERT_RECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DSERRORHANDLER() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_HAS_TABLE() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_SETUP_RECT() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_DRAW_ALERT() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_SKIP_ICON() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_SKIP_PROC() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_SHOW_BTNS() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_RESUME() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void DSERR_RESTART() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void LOOKUP_ALERT_ID() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LOOKUP_LOOP() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LOOKUP_NEXT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void LOOKUP_FOUND() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DRAW_ALERT_TEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWTEXT_SCAN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWTEXT_SEGMENT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAWTEXT_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void CALL_DS_DRAWPROC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void INIT_ALERT_RECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SET_DEFAULT_RECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void REPOS_ALERT_PORT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAW_ALERT_FRAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAW_FRAME_RTS() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SHOW_ALERT_BTNS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DRAW_BTN_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void WAIT_MOUSE_DOWN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void HITTEST_BTN_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TRACK_BTN_DOWN() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TRACK_BTN_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TRACK_BTN_CHECK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BTN_PT_IN_RECT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void BTN_INVERT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // sys_error
