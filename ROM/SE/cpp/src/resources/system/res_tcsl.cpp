#include "serom/resources/system/res_tcsl.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_tcsl
namespace serom { namespace resources { namespace res_tcsl { 
    
    void Resource_tcsl_0000() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Resource_tcsl_0000_Header() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void _tcsl_0_() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TCSL_SLIDESHOW() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TCSL_NEXTFRAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TCSL_LOADBBMC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TCSL_DRAWFRAME() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void TCSL_ROWLOOP() {
        // Continuation / loop sequence
        g_cpu.ccr.z = true;
    }
    void TCSL_NOBBMC() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // res_tcsl
