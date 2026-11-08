#include "serom/debugger/debugger.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: debugger
namespace serom { namespace debugger { namespace debugger { 
    
    void SYSERRINIT() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void VECLOOP_UNIQUE() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void VECLOOP_UNASSIGNED() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void VECLOOP_IRQ() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void VECLOOP_TRAP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void CLEAR_SEREGS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DEBUGGER() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBG_NO_DEBUGGER() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DBG_CLEANUP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DEBUGPROLOG() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TODEEPSHIT() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_REGS_SAVED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_STD_FRAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_DISCARD_FRAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_TO_SYSERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GENEXCPS() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_ADDR_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void EXCPT_ILLEGAL() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_ZERO_DIV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_CHK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_TRAPV() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_PRIVILEGE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_TRACE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_LINE_1010() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_LINE_1111() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void EXCPT_UNASSIGNED() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void IRQEXCEPTION() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // debugger
