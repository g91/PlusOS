#include "serom/os/file/file_mgr_dir.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: file_mgr_dir
namespace serom { namespace os { namespace file_mgr_dir { 
    void GETFILEINFO_OK_6() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_LOOP_5() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_ERR_4() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_NO_5() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_NO_6() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_OK_7() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void SETFILEINFO_MFS_FLUSH() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_ERR_5() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_NO_7() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_NO_8() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_ERR_6() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_CONT_8() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_OK_8() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_OK_9() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_CONT_9() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_SKIP_3() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void SETFILEINFO_MFS_NEXT_BLOCK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_FOUND_3() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_ERR_7() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_OK_10() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_OK_11() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_CONT_10() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_CONT_11() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_OK_12() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_ERR_8() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
    void GETFILEINFO_OK_13() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void GETFILEINFO_CONT_12() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_NO_10() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_LOOP_6() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_LOOP_7() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void GETFILEINFO_NO_11() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TFSBitTst() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void GETFILEINFO_OK_15() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void TFSVCBTST() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void SEEK() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TFSVCBTST_CONT() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TFSVCBTST_CONT_2() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TFSVCBTST_OK() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void TFSVCBTST_OK_2() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void TFSVCBTST_OK_3() {
        // Successful exit
        D(0) = 0;
        g_cpu.ccr.z = true;
        g_cpu.ccr.n = false;
    }
    void TFSVCBTST_ERR() {
        // Error exit
        D(0) = static_cast<UInt32>(-1);
        g_cpu.ccr.z = false;
        g_cpu.ccr.n = true;
    }
}}}  // file_mgr_dir
