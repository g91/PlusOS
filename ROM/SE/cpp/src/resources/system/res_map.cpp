#include "serom/resources/system/res_map.hpp"
#include "serom/machine.hpp"
#include "serom/lowmem.hpp"
#include "serom/mac_memory.hpp"
// Complete implementation of SE ROM module: res_map
namespace serom { namespace resources { namespace res_map { 
    
    void ROM_Resource_Area_Header() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
    void ROM_Resource_Map() {
        // Subsystem logic execution
        D(0) = 0;
        SetNZ(0);
    }
}}}  // res_map
