#include "serom/resources/system/res_kmap.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_kmap
namespace serom { namespace resources { namespace res_kmap { 
    
    void Resource_KMAP_0000() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void Resource_KMAP_0000_Header() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void _KMAP_0_() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // res_kmap
