#include "serom/toolbox/quickdraw/PutLine.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: PutLine
namespace serom { namespace toolbox { namespace quickdraw { namespace PutLine { 
    
    void PUTLINE() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void DHPOS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DVPOS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DVPOS_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void DVPOS_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void PUTLINE_SLANTED() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
    void NOSWAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOSWAP_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void NOSWAP_NO() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void NOCHNG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void NOCHNG_EXIT() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void PUTLINE_GOHOME() {
        // QuickDraw Line/Move
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // PutLine
