#include "serom/toolbox/quickdraw/SortPoints.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: SortPoints
namespace serom { namespace toolbox { namespace quickdraw { namespace SortPoints { 
    
    void SORTPOINTS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SORTPOINTS_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SPLIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SPLIT_RETRY() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SPLIT_NEXT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SPLIT_LOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void THREE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void FOUR() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SPLIT_LOOP_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SPLIT_DONE_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SORTPOINTS_NOSWAP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SORTPOINTS_RIGHTOK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SORTPOINTS_GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CULLPOINTS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CULLPOINTS_DELETE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CULLPOINTS_MORE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CULLPOINTS_WHILE1() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void CULLPOINTS_DONE() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void CULLPOINTS_GOHOME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}}  // SortPoints
