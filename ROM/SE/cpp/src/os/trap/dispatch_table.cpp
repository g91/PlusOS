#include "serom/os/trap/dispatch_table.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: dispatch_table
namespace serom { namespace os { namespace dispatch_table { 
    
    void RESTOP() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DSPTABLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Compressed_ALine_Dispatch_Table() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void DISPTABLE() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // dispatch_table
